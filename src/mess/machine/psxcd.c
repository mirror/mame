#include "emu.h"
#include "psxcd.h"
#include "debugger.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine& machine, int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%s: %s", machine.describe_context(), buf );
	}
}

enum cdrom_events
{
	event_cmd_complete=0,
	event_read_sector,
	event_play_sector,
	event_change_disk
};

enum intr_status
{
	intr_nointr=0,
	intr_dataready,
	intr_acknowledge,
	intr_complete,
	intr_dataend,
	intr_diskerror
};

enum mode_flags
{
	mode_double_speed=0x80,
	mode_adpcm=0x40,
	mode_size=0x20,
	mode_size2=0x10,
	mode_size_mask=0x30,
	mode_channel=0x08,
	mode_report=0x04,
	mode_autopause=0x02,
	mode_cdda=0x01
};

enum status_f
{
	status_playing=0x80,
	status_seeking=0x40,
	status_reading=0x20,
	status_shellopen=0x10,
	status_invalid=0x08,
	status_seekerror=0x04,
	status_standby=0x02,
	status_error=0x01
};

struct subheader
{
	UINT8 file, channel, submode, coding;
};

enum submode_flags
{
	submode_eof=0x80,
	submode_realtime=0x40,
	submode_form=0x20,
	submode_trigger=0x10,
	submode_data=0x08,
	submode_audio=0x04,
	submode_video=0x02,
	submode_eor=0x01
};

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type PSXCD = &device_creator<psxcd_device>;

psxcd_device::psxcd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	cdrom_image_device(mconfig, PSXCD, "PSX Cdrom", tag, owner, clock, "psx_cd", __FILE__),
	m_irq_handler(*this)
{
	static_set_interface(*this, "psx_cdrom");
}


void psxcd_device::device_start()
{
	cdrom_image_device::device_start();
	m_irq_handler.resolve_safe();

	m_maincpu = machine().device<cpu_device>("maincpu");
	m_spu = machine().device<spu_device>("spu");

	unsigned int sysclk=m_maincpu->clock()/2;
	start_read_delay=(sysclk/60);
	read_sector_cycles=(sysclk/75);
	preread_delay=(read_sector_cycles>>2)-500;

	m_sysclock = sysclk;

	res_queue = NULL;
	rdp = 0;
	status=status_shellopen;
	mode=0;

	for (int i = 0; i < MAX_PSXCD_TIMERS; i++)
	{
		m_timers[i] = timer_alloc(i);
		m_timerinuse[i] = false;
	}

	save_item(NAME(cmdbuf));
	save_item(NAME(mode));
	save_item(NAME(secbuf));
	save_item(NAME(filter_file));
	save_item(NAME(filter_channel));
	save_item(NAME(lastsechdr));
	save_item(NAME(status));
	save_item(NAME(rdp));
	save_item(NAME(m_cursec));
	save_item(NAME(sectail));
	save_item(NAME(m_transcurr));
	save_item(NAME(m_transbuf));
	save_item(NAME(loc.w));
	save_item(NAME(curpos.w));
	save_item(NAME(open));
	save_item(NAME(m_mute));
	save_item(NAME(m_dmaload));
	save_item(NAME(next_read_event));
	save_item(NAME(next_sector_t));
	save_item(NAME(autopause_sector));
	save_item(NAME(m_param_count));
}

void psxcd_device::device_stop()
{
	for (int i = 0; i < MAX_PSXCD_TIMERS; i++)
	{
		if(m_timerinuse[i] && m_timers[i]->ptr())
			global_free((command_result *)m_timers[i]->ptr());
	}
	while(res_queue)
	{
		command_result *res = res_queue->next;
		global_free(res_queue);
		res_queue = res;
	}
}

void psxcd_device::device_reset()
{
	next_read_event = -1;
	stop_read();

	for (int i = 0; i < MAX_PSXCD_TIMERS; i++)
	{
		if(m_timerinuse[i] && m_timers[i]->ptr())
			global_free((command_result *)m_timers[i]->ptr());
		m_timers[i]->adjust(attotime::never, 0, attotime::never);
		m_timerinuse[i] = false;
	}
	open = true;
	if(m_cdrom_handle)
		add_system_event(event_change_disk, m_sysclock, NULL);

	while(res_queue)
	{
		command_result *res = res_queue->next;
		global_free(res_queue);
		res_queue = res;
	}

	m_param_count = 0;
	m_regs.sr = 0x18;
	m_regs.ir = 0;
	m_regs.imr = 0x1f;
	sectail = 0;
	m_cursec = 0;
	m_mute = false;
	m_dmaload = false;
	m_int1 = NULL;
	curpos.w = 0;
}

bool psxcd_device::call_load()
{
	bool ret = cdrom_image_device::call_load();
	open = true;
	if(ret == IMAGE_INIT_PASS)
		add_system_event(event_change_disk, m_sysclock, NULL); // 1 sec to spin up the disk
	return ret;
}

void psxcd_device::call_unload()
{
	stop_read();
	cdrom_image_device::call_unload();
	open = true;
	status = status_shellopen;
	send_result(intr_diskerror);
}

READ8_MEMBER( psxcd_device::read )
{
	UINT8 ret = 0;
	switch (offset & 3)
	{
		/*
		x--- ---- command/parameter busy flag
		-x-- ---- data fifo full (active low)
		--x- ---- response fifo empty (active low)
		---x ---- parameter fifo full (active low)
		---- x--- parameter fifo empty (active high)
		---- --xx cmd mode
		*/
		case 0:
			ret = m_regs.sr;
			break;

		case 1:
			if ((res_queue) && (rdp < res_queue->sz))
			{
				ret = res_queue->data[rdp++];
				if(rdp == res_queue->sz)
					m_regs.sr &= ~0x20;
				else
					m_regs.sr |= 0x20;
			}
			else
				ret = 0;
			break;

		case 2:
			if(!m_dmaload)
				ret = 0;
			else
			{
				ret = m_transbuf[m_transcurr++];
				if(m_transcurr >= raw_sector_size)
				{
					m_dmaload = false;
					m_regs.sr &= ~0x40;
				}
			}
			break;

		case 3:
			if(m_regs.sr & 1)
				ret = m_regs.ir | 0xe0;
			else
				ret = m_regs.imr | 0xe0;
			break;
	}

	verboselog(machine(), 2, "psxcd: read byte %08x = %02x\n",offset,ret);

	return ret;
}

WRITE8_MEMBER( psxcd_device::write )
{
	verboselog(machine(), 2, "psxcd: write byte %08x = %02x\n",offset,data);

	switch ((offset & 3) | ((m_regs.sr & 3) << 4))
	{
		case 0x00:
		case 0x10:
		case 0x20:
		case 0x30:
			m_regs.sr = (m_regs.sr & ~3) | (data & 3);
			break;

		case 0x01:
			write_command(data);
			break;

		case 0x11:
		case 0x21:
			break;

		case 0x31:
			m_regs.vol.rr = data;
			break;

		case 0x02:
			cmdbuf[m_param_count] = data;
			m_param_count++;
			break;

		case 0x12:
			m_regs.imr = data & 0x1f;
			break;

		case 0x22:
			m_regs.vol.ll = data;
			break;

		case 0x32:
			m_regs.vol.rl = data;
			break;
		/*
		x--- ---- unknown
		-x-- ---- Reset parameter FIFO
		--x- ---- unknown (used on transitions, so it certainly resets something)
		---x ---- Command start
		---- -xxx Response received
		*/
		case 0x03:
			if((data & 0x80) && !m_dmaload)
			{
				m_dmaload = true;
				memcpy(m_transbuf, secbuf[m_cursec], raw_sector_size);
				m_regs.sr |= 0x40;

				m_cursec++;
				m_cursec %= sector_buffer_size;

				switch(mode & mode_size_mask)
				{
					case 0x00:
					default:
						m_transcurr = 24;
						break;
					case 0x10:
						m_transcurr = 24;
						break;
					case 0x20:
						m_transcurr = 12;
						break;
				}
#if (VERBOSE_LEVEL > 0)
				char str[1024];
				for (int i=0; i<12; i++)
					sprintf(&str[i*4], "%02x  ", m_transbuf[i+12]);
				verboselog(machine(), 1, "psxcd: request data=%s\n",str);
#endif
			}
			else if(!(data & 0x80))
			{
				m_dmaload = false;
				m_regs.sr &= ~0x40;
			}
			break;

		case 0x13:
			if(data & 0x1f)
			{
				m_regs.ir &= ~(data & 0x1f);

				if(res_queue && !m_regs.ir)
				{
					command_result *res = res_queue;
					if(res == m_int1)
						m_int1 = NULL;

					res_queue = res->next;
					global_free(res);
					m_regs.sr &= ~0x20;
					rdp = 0;
					if(res_queue)
					{
						m_regs.sr |= 0x20;
						m_regs.ir = res_queue->res;
					}
					verboselog(machine(), 1, "psxcd: nextres\n");
				}
			}
			if(data & 0x40)
				m_param_count = 0;
			break;

		case 0x23:
			m_regs.vol.lr = data;
			break;

		case 0x33:
			break;
	}
}

const psxcd_device::cdcmd psxcd_device::cmd_table[]=
{
	&psxcd_device::cdcmd_sync,
	&psxcd_device::cdcmd_nop,
	&psxcd_device::cdcmd_setloc,
	&psxcd_device::cdcmd_play,
	&psxcd_device::cdcmd_forward,
	&psxcd_device::cdcmd_backward,
	&psxcd_device::cdcmd_readn,
	&psxcd_device::cdcmd_standby,
	&psxcd_device::cdcmd_stop,
	&psxcd_device::cdcmd_pause,
	&psxcd_device::cdcmd_init,
	&psxcd_device::cdcmd_mute,
	&psxcd_device::cdcmd_demute,
	&psxcd_device::cdcmd_setfilter,
	&psxcd_device::cdcmd_setmode,
	&psxcd_device::cdcmd_getparam,
	&psxcd_device::cdcmd_getlocl,
	&psxcd_device::cdcmd_getlocp,
	&psxcd_device::cdcmd_unknown12,
	&psxcd_device::cdcmd_gettn,
	&psxcd_device::cdcmd_gettd,
	&psxcd_device::cdcmd_seekl,
	&psxcd_device::cdcmd_seekp,
	&psxcd_device::cdcmd_illegal17,
	&psxcd_device::cdcmd_illegal18,
	&psxcd_device::cdcmd_test,
	&psxcd_device::cdcmd_id,
	&psxcd_device::cdcmd_reads,
	&psxcd_device::cdcmd_reset,
	&psxcd_device::cdcmd_illegal1d,
	&psxcd_device::cdcmd_readtoc
};

void psxcd_device::write_command(UINT8 byte)
{
	if(byte > 31)
		illegalcmd(byte);
	else
		(this->*cmd_table[byte])();
	m_param_count = 0;
}

void psxcd_device::cdcmd_sync()
{
	verboselog(machine(), 1, "psxcd: sync\n");

	stop_read();
	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_nop()
{
	verboselog(machine(), 1, "psxcd: nop\n");

	if (!open)
		status &= ~status_shellopen;

	send_result(intr_complete);
}

void psxcd_device::cdcmd_setloc()
{
	verboselog(machine(), 1, "psxcd: setloc %08x:%08x:%08x\n", cmdbuf[0], cmdbuf[1], cmdbuf[2]);

	stop_read();


	CDPOS l;
	l.b[M]=bcd_to_decimal(cmdbuf[0]);
	l.b[S]=bcd_to_decimal(cmdbuf[1]);
	l.b[F]=bcd_to_decimal(cmdbuf[2]);

	if ((l.b[M]>0) || (l.b[S]>=2))
		loc.w=l.w;
	else
		verboselog(machine(), 0, "psxcd: setloc out of range: %02d:%02d:%02d\n",l.b[M],l.b[S],l.b[F]);

	send_result(intr_complete);
}

void psxcd_device::cdcmd_play()
{
	if(cmdbuf[0] && m_param_count)
			loc.w = lba_to_msf_ps(cdrom_get_track_start(m_cdrom_handle, bcd_to_decimal(cmdbuf[0]) - 1));

	curpos.w = loc.w;
	if (!curpos.w)
		curpos.b[S] = 2;

	verboselog(machine(), 1, "psxcd: play %02x %02x %02x => %d\n", decimal_to_bcd(loc.b[M]), decimal_to_bcd(loc.b[S]), decimal_to_bcd(loc.b[F]), msf_to_lba_ps(loc.w));

	stop_read();
	start_play();
	send_result(intr_complete);
}

void psxcd_device::cdcmd_forward()
{
	verboselog(machine(), 1, "psxcd: forward\n");
}

void psxcd_device::cdcmd_backward()
{
	verboselog(machine(), 1, "psxcd: backward\n");
}

void psxcd_device::cdcmd_readn()
{
	if(!open)
	{
		verboselog(machine(), 1, "psxcd: readn\n");

		curpos.w=loc.w;

		stop_read();
		start_read();
	} else
	{
		send_result(intr_diskerror, NULL, 0, 0x80);
	}
}

void psxcd_device::cdcmd_standby()
{
	verboselog(machine(), 1, "psxcd: standby\n");

	stop_read();
	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_stop()
{
	verboselog(machine(), 1, "psxcd: stop\n");

	stop_read();
	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_pause()
{
	verboselog(machine(), 1, "psxcd: pause\n");

	stop_read();

	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_init()
{
	verboselog(machine(), 1, "psxcd: init\n");

	stop_read();
	mode=0;
	m_regs.sr |= 0x10;

	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_mute()
{
	verboselog(machine(), 1, "psxcd: mute\n");

	m_mute = true;
	send_result(intr_complete);
}

void psxcd_device::cdcmd_demute()
{
	verboselog(machine(), 1, "psxcd: demute\n");

	m_mute = false;
	send_result(intr_complete);
}

void psxcd_device::cdcmd_setfilter()
{
	verboselog(machine(), 1, "psxcd: setfilter %08x,%08x\n",cmdbuf[0],cmdbuf[1]);

	filter_file=cmdbuf[0];
	filter_channel=cmdbuf[1];

	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_setmode()
{
	verboselog(machine(), 1, "psxcd: setmode %08x\n",cmdbuf[0]);

	mode=cmdbuf[0];
	send_result(intr_complete);
}

void psxcd_device::cdcmd_getparam()
{
	unsigned char data[6]=
	{
		status,
		mode,
		filter_file,
		filter_channel,
		0,
		0
	};

	verboselog(machine(), 1, "psxcd: getparam [%02x %02x %02x %02x %02x %02x]\n",
								data[0], data[1], data[2], data[3], data[4], data[5]);

	send_result(intr_complete,data,6);
}

UINT32 psxcd_device::sub_loc(CDPOS src1, CDPOS src2)
{
	CDPOS dst;
	int f=src1.b[F]-src2.b[F],
			s=src1.b[S]-src2.b[S],
			m=src1.b[M]-src2.b[M];
	while (f<0) { s--; f+=75; }
	while (s<0) { m--; s+=60; }

	if (m<0)
		m=s=f=0;

	dst.b[M]=m;
	dst.b[S]=s;
	dst.b[F]=f;

	return dst.w;
}

void psxcd_device::cdcmd_getlocl()
{
	verboselog(machine(), 1, "psxcd: getlocl [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
							lastsechdr[0], lastsechdr[1], lastsechdr[2], lastsechdr[3],
							lastsechdr[4], lastsechdr[5], lastsechdr[6], lastsechdr[7]);

	send_result(intr_complete,lastsechdr,8);
}

void psxcd_device::cdcmd_getlocp()
{
	CDPOS tloc, start;
	UINT8 track = cdrom_get_track(m_cdrom_handle, msf_to_lba_ps(loc.w)) + 1;
	start.w = (track == 1) ? 0x000200 : lba_to_msf_ps(cdrom_get_track_start(m_cdrom_handle, track - 1));
	tloc.w = sub_loc(loc, start);

	unsigned char data[8]=
	{
		decimal_to_bcd(track),                          // track
		0x01,                           // index
		decimal_to_bcd(tloc.b[M]),    // min
		decimal_to_bcd(tloc.b[S]),    // sec
		decimal_to_bcd(tloc.b[F]),    // frame
		decimal_to_bcd(loc.b[M]), // amin
		decimal_to_bcd(loc.b[S]), // asec
		decimal_to_bcd(loc.b[F])  // aframe
	};

	verboselog(machine(), 1, "psxcd: getlocp [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
						data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	send_result(intr_complete,data,8);
}

void psxcd_device::cdcmd_gettn()
{
	verboselog(machine(), 1, "psxcd: gettn\n");


	if(!open)
	{
		unsigned char data[3]=
		{
			status,
			decimal_to_bcd(1),
			decimal_to_bcd(cdrom_get_last_track(m_cdrom_handle))
		};

		send_result(intr_complete,data,3);
	}
	else
	{
		send_result(intr_diskerror, NULL, 0, 0x80);
	}
}

void psxcd_device::cdcmd_gettd()
{
	UINT8 track = bcd_to_decimal(cmdbuf[0]);
	UINT8 last = cdrom_get_last_track(m_cdrom_handle);
	if(track <= last)
	{
		CDPOS trkstart;
		if(!track) // length of disk
			trkstart.w = lba_to_msf_ps(cdrom_get_track_start(m_cdrom_handle, 0xaa));
		else
			trkstart.w = lba_to_msf_ps(cdrom_get_track_start(m_cdrom_handle, track - 1));

		unsigned char data[3]=
		{
			status,
			decimal_to_bcd(trkstart.b[M]),
			decimal_to_bcd(trkstart.b[S])
		};

		verboselog(machine(), 1, "psxcd: gettd %02x [%02x %02x %02x]\n", cmdbuf[0], data[0], data[1], data[2]);

		send_result(intr_acknowledge,data,3);
	}
	else
	{
		send_result(intr_diskerror, NULL, 0, 0x80);
	}
}

void psxcd_device::cdcmd_seekl()
{
	verboselog(machine(), 1, "psxcd: seekl [%02d:%02d:%02d]\n", loc.b[M], loc.b[S], loc.b[F]);

	curpos.w=loc.w;

	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_seekp()
{
	verboselog(machine(), 1, "psxcd: seekp\n");

	curpos.w=loc.w;

	send_result(intr_acknowledge);
}

void psxcd_device::cdcmd_test()
{
	verboselog(machine(), 1, "psxcd: test %02x\n", cmdbuf[0]);

	switch(cmdbuf[0])
	{
		case 0x20:
		{
			static UINT8 data[4]=
			{
				0x95,
				0x07,
				0x06,
				0xff
			};

			send_result(intr_complete,data,4);
			break;
		}

		default:
			verboselog(machine(), 0, "psxcd: unimplemented test cmd %02x\n", cmdbuf[0]);
			cmd_complete(prepare_result(intr_diskerror, NULL, 0, 0x10));
			break;
	}
}

void psxcd_device::cdcmd_id()
{
	verboselog(machine(), 1, "psxcd: id\n");

	if (!open)
	{
		UINT8 cdid[8];
		int irq;
		memset(cdid, '\0', 8);

		send_result(intr_complete);
		if(cdrom_get_track_type(m_cdrom_handle, 0) == CD_TRACK_AUDIO)
		{
			irq = intr_diskerror;
			cdid[0] = status | status_invalid;
			cdid[1] = 0x90;
		}
		else
		{
			irq = intr_acknowledge;
			cdid[0] = status;
			cdid[4] = 'S';
			cdid[5] = 'C';
			cdid[6] = 'E';
			cdid[7] = 'A';
		}
		send_result(irq,cdid,8,default_irq_delay*3);
	}
	else
	{
		send_result(intr_diskerror, NULL, 0, 0x80);
	}
}

void psxcd_device::cdcmd_reads()
{
	if(!open)
	{
		verboselog(machine(), 1, "psxcd: reads\n");

		curpos.w=loc.w;

		stop_read();
		start_read();
	} else
	{
		send_result(intr_diskerror, NULL, 0, 0x80);
	}
}

void psxcd_device::cdcmd_reset()
{
	verboselog(machine(), 1, "psxcd: reset\n");
}

void psxcd_device::cdcmd_readtoc()
{
	verboselog(machine(), 1, "psxcd: readtoc\n");

	send_result(intr_complete);
	send_result(intr_acknowledge, NULL, 0, default_irq_delay*3); // ?
}

void psxcd_device::cdcmd_unknown12()
{
	verboselog(machine(), 1, "psxcd: unknown 12\n");
	// set session? readt?
	if(cmdbuf[0] == 1)
		send_result(intr_complete);
	else
		send_result(intr_diskerror, NULL, 0, 0x40);
}

void psxcd_device::cdcmd_illegal17()
{
	illegalcmd(0x17); // set clock?
}

void psxcd_device::cdcmd_illegal18()
{
	illegalcmd(0x18); // get clock?
}

void psxcd_device::cdcmd_illegal1d()
{
	illegalcmd(0x1d); // read q subchannel
}

void psxcd_device::illegalcmd(UINT8 cmd)
{
	verboselog(machine(), 0, "psxcd: unimplemented cd command %02x\n", cmd);

	send_result(intr_diskerror, NULL, 0, 0x40);
}

void psxcd_device::cmd_complete(command_result *res)
{
	command_result *rf;

	verboselog(machine(), 1, "psxcd: irq [%d]\n", res->res);

	if (res_queue)
	{
		for (rf=res_queue; rf->next; rf=rf->next);
		rf->next=res;
	}
	else
	{
		res_queue = res;
		m_regs.ir = res_queue->res;
		if(m_regs.ir & m_regs.imr)
			m_irq_handler(1);
		m_regs.sr |= 0x20;
	}
	res->next = NULL;
}

psxcd_device::command_result *psxcd_device::prepare_result(UINT8 res, UINT8 *data, int sz, UINT8 errcode)
{
	command_result *cr=global_alloc(command_result);

	cr->res=res;
	if (sz)
	{
		assert(sz<sizeof(cr->data));
		memcpy(cr->data,data,sz);
		cr->sz=sz;
	} else
	{
		if((res == intr_diskerror) && errcode)
		{
			cr->data[0] = status | status_error;
			cr->data[1] = errcode;
			cr->sz = 2;
		}
		else
		{
			cr->data[0]=status;
			cr->sz=1;
		}
	}
	status &= ~status_error;

	return cr;
}

void psxcd_device::send_result(UINT8 res, UINT8 *data, int sz, int delay, UINT8 errcode)
{
	// Avoid returning results after sector read results -
	// delay the sector read slightly if necessary

	UINT64 systime = m_maincpu->total_cycles();
	if ((next_read_event != -1) && ((systime+delay)>(next_sector_t)))
	{
		UINT32 hz = m_sysclock / (delay + 2000);
		m_timers[next_read_event]->adjust(attotime::from_hz(hz), 0, attotime::never);
	}

	add_system_event(event_cmd_complete, delay, prepare_result(res, data, sz, errcode));
}

void psxcd_device::start_dma(UINT8 *mainram, UINT32 size)
{
	UINT32 sector_size;
	verboselog(machine(), 1, "psxcd: start dma %d bytes at %d\n", size, m_transcurr);

	if(!m_dmaload)
		return;

	switch(mode & mode_size_mask)
	{
		case 0x00:
		default:
			sector_size = 2048 + 24;
			break;
		case 0x10:
			sector_size = 2328 + 12;
			break;
		case 0x20:
			sector_size = 2340 + 12;
			break;
	}

	if(size > (sector_size - m_transcurr))
		size = (sector_size - m_transcurr);

	memcpy(mainram, &m_transbuf[m_transcurr], size);
	m_transcurr += size;

	if(sector_size <= m_transcurr)
	{
		m_dmaload = false;
		m_regs.sr &= ~0x40;
	}
}

void psxcd_device::read_sector()
{
	next_read_event=-1;

	if (status & status_reading)
	{
		bool isend = false;
		UINT32 sector = msf_to_lba_ps(curpos.w);
		UINT8 *buf = secbuf[sectail];
		if (cdrom_read_data(m_cdrom_handle, sector, buf, CD_TRACK_RAW_DONTCARE))
		{
			subheader *sub=(subheader *)(buf+16);
			memcpy(lastsechdr, buf+12, 8);

			verboselog(machine(), 2, "psxcd: subheader file=%02x chan=%02x submode=%02x coding=%02x [%02x%02x%02x%02x]\n",
						sub->file, sub->channel, sub->submode, sub->coding, buf[0xc], buf[0xd], buf[0xe], buf[0xf]);

			if ((mode & mode_adpcm) && (sub->submode & submode_audio))
			{
				if ((sub->submode & submode_eof) && (mode & mode_autopause))
				{
					isend=true;
					//printf("end of file\n");
				}
				if((!(mode & mode_channel) ||
						((sub->file == filter_file) && (sub->channel == filter_channel))) && !m_mute)
					m_spu->play_xa(0,buf+16);
			}
			else
			{
				if(!m_int1)
					m_cursec = sectail;

				m_int1 = prepare_result(intr_dataready);
				cmd_complete(m_int1);
				sectail++;
				sectail %= sector_buffer_size;
			}

			curpos.b[F]++;
			if (curpos.b[F]==75)
			{
				curpos.b[F]=0;
				curpos.b[S]++;
				if (curpos.b[S]==60)
				{
					curpos.b[S]=0;
					curpos.b[M]++;
				}
			}

			loc.w=curpos.w;
		}
		else
			isend = true;

		if (! isend)
		{
			unsigned int cyc=read_sector_cycles;
			if (mode&mode_double_speed) cyc>>=1;

			next_sector_t+=cyc;

			next_read_event = add_system_event(event_read_sector, cyc, NULL);
		} else
		{
			verboselog(machine(), 1, "psxcd: autopause xa\n");

			cmd_complete(prepare_result(intr_dataend));
			stop_read();
		}
	}
}

void psxcd_device::play_sector()
{
	next_read_event=-1;

	if (status&status_playing)
	{
		UINT32 sector = msf_to_lba_ps(curpos.w);

		if(cdrom_read_data(m_cdrom_handle, sector, secbuf[sectail], CD_TRACK_AUDIO))
		{
			if(!m_mute)
				m_spu->play_cdda(0, secbuf[sectail]);
		}
		else
		{
			if(!cdrom_read_data(m_cdrom_handle, sector, secbuf[sectail], CD_TRACK_RAW_DONTCARE))
			{
				stop_read(); // assume we've reached the end
				cmd_complete(prepare_result(intr_dataend));
				return;
			}
		}

		curpos.b[F]++;
		if (curpos.b[F]==75)
		{
			curpos.b[F]=0;
			curpos.b[S]++;
			if (curpos.b[S]==60)
			{
				curpos.b[S]=0;
				curpos.b[M]++;
			}
		}

		loc.w=curpos.w;
		sector++;
		sectail++;
		sectail %= sector_buffer_size;

		if (mode&mode_autopause)
		{
			if (sector>=autopause_sector)
			{
				verboselog(machine(), 1, "psxcd: autopause cdda\n");

				stop_read();
				cmd_complete(prepare_result(intr_dataend));
				return;
			}
		}

		if ((mode&mode_report) && !(sector & 15)) // slow the int rate
		{
			command_result *res=global_alloc(command_result);
			UINT8 track = cdrom_get_track(m_cdrom_handle, sector) + 1;
			res->res=intr_dataready;

			res->data[0]=status;
			res->data[1]=decimal_to_bcd(track);
			res->data[2]=1;
			if(sector & 0x10)
			{
				CDPOS tloc, start;
				start.w = (track == 1) ? 0x000200 : lba_to_msf_ps(cdrom_get_track_start(m_cdrom_handle, track - 1));
				tloc.w = sub_loc(loc, start);
				res->data[3]=decimal_to_bcd(tloc.b[M]);
				res->data[4]=decimal_to_bcd(tloc.b[S]) | 0x80;
				res->data[5]=decimal_to_bcd(tloc.b[F]);
			}
			else
			{
				res->data[3]=decimal_to_bcd(loc.b[M]);
				res->data[4]=decimal_to_bcd(loc.b[S]);
				res->data[5]=decimal_to_bcd(loc.b[F]);
			}

			res->sz=8;

			cmd_complete(res);
		}

		unsigned int cyc=read_sector_cycles;

		next_sector_t+=cyc>>1;

		next_read_event = add_system_event(event_play_sector, next_sector_t - m_maincpu->total_cycles(), NULL);
	}
}

void psxcd_device::start_read()
{
	UINT32 sector = msf_to_lba_ps(curpos.w);

	assert((status&(status_reading|status_playing))==0);

	if(!(mode & mode_cdda) && (cdrom_get_track_type(m_cdrom_handle, cdrom_get_track(m_cdrom_handle, sector + 150)) == CD_TRACK_AUDIO))
	{
		stop_read();
		cmd_complete(prepare_result(intr_diskerror, NULL, 0, 0x40));
		return;
	}
	send_result(intr_complete);
	status |= status_reading;

	m_cursec=sectail=0;

	unsigned int cyc=read_sector_cycles;
	if (mode&mode_double_speed) cyc>>=1;

	INT64 systime=m_maincpu->total_cycles();

	systime+=start_read_delay;

	next_sector_t=systime+cyc;
	next_read_event = add_system_event(event_read_sector, start_read_delay+preread_delay, NULL);
}

void psxcd_device::start_play()
{
	UINT8 track = cdrom_get_track(m_cdrom_handle, msf_to_lba_ps(curpos.w) + 150);

	if(cdrom_get_track_type(m_cdrom_handle, track) != CD_TRACK_AUDIO)
		verboselog(machine(), 0, "psxcd: playing data track\n");

	status|=status_playing;

	m_cursec=sectail=0;

	if (mode&mode_autopause)
	{
		autopause_sector = cdrom_get_track_start(m_cdrom_handle, track) + cdrom_get_toc(m_cdrom_handle)->tracks[track].frames;
//      printf("pos=%d auto=%d\n",pos,autopause_sector);
	}

	unsigned int cyc=read_sector_cycles;

	next_sector_t=m_maincpu->total_cycles()+cyc;

	next_sector_t+=cyc>>1;

	next_read_event = add_system_event(event_play_sector, next_sector_t - m_maincpu->total_cycles(), NULL);
}

void psxcd_device::stop_read()
{
	if (status & (status_reading|status_playing))
		verboselog(machine(), 1, "psxcd: stop read\n");

	status&=~(status_reading|status_playing);

	if (next_read_event != -1)
	{
		m_timers[next_read_event]->adjust(attotime::never, 0, attotime::never);
		m_timerinuse[next_read_event] = false;
		next_read_event = -1;
	}

	UINT32 sector=msf_to_lba_ps(curpos.w);
	m_spu->flush_xa(sector);
	m_spu->flush_cdda(sector);
}

void psxcd_device::device_timer(emu_timer &timer, device_timer_id tid, int param, void *ptr)
{
	if (!m_timerinuse[tid])
	{
		verboselog(machine(), 0, "psxcd: timer fired for free event\n");
		return;
	}

	m_timerinuse[tid] = false;
	switch (param)
	{
		case event_cmd_complete:
		{
			verboselog(machine(), 1, "psxcd: event cmd complete\n");

			cmd_complete((command_result *)ptr);
			break;
		}

		case event_read_sector:
			read_sector();
			break;

		case event_play_sector:
			play_sector();
			break;

		case event_change_disk:
			open = false;
			status |= status_standby;
			break;
	}
}

int psxcd_device::add_system_event(int type, UINT64 t, command_result *ptr)
{
	// t is in maincpu clock cycles
	UINT32 hz = m_sysclock / t;
	for(int i = 0; i < MAX_PSXCD_TIMERS; i++)
	{
		if(!m_timerinuse[i])
		{
			m_timers[i]->adjust(attotime::from_hz(hz), type, attotime::never);
			m_timers[i]->set_ptr(ptr);
			m_timerinuse[i] = true;
			return i;
		}
	}
	fatalerror("psxcd: out of timers\n");
}
