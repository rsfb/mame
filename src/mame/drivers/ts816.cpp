// license:BSD-3-Clause
// copyright-holders:Robbbert
/***************************************************************************

2013-09-10 Skeleton driver for Televideo TS816

TODO:
- Connect up the devices to each other
- Connect up RS232 terminal instead of parallel one
- Connect centronics printer to PIO
- 4 diagnostic LEDs
- Hard Drive
- Tape Drive
- Get a good dump of the rom. If the undocumented DSW is enabled, it
  calls up code in the missing half of the rom. Also it isn't possible
  at the moment to get any useful response to commands.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"
#include "machine/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80dma.h"

class ts816_state : public driver_device
{
public:
	ts816_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_terminal(*this, "terminal")
	{ }

	void kbd_put(u8 data);
	DECLARE_READ8_MEMBER(keyin_r);
	DECLARE_READ8_MEMBER(status_r);
	DECLARE_WRITE8_MEMBER(port68_w);
	DECLARE_WRITE8_MEMBER(port78_w);
	DECLARE_WRITE8_MEMBER(porte0_w);
	DECLARE_WRITE8_MEMBER(portf0_w);
	void init_ts816();

	void ts816(machine_config &config);
	void ts816_io(address_map &map);
	void ts816_mem(address_map &map);
private:
	uint8_t m_term_data;
	uint8_t m_status;
	bool m_2ndbank;
	bool m_endram;
	void set_banks();
	virtual void machine_reset() override;
	required_device<cpu_device> m_maincpu;
	required_device<generic_terminal_device> m_terminal;
};

void ts816_state::ts816_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x3fff).bankr("bankr0").bankw("bankw0");
	map(0x4000, 0xdfff).bankrw("bank1");
	map(0xe000, 0xffff).bankrw("bank2");
}

void ts816_state::ts816_io(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x00); // Tape status byte 1
	map(0x01, 0x01); // Tape status byte 2 and diagnostics mode
	map(0x02, 0x02); // Hard Disk status
	map(0x03, 0x03); // Hard Disk output latch
	map(0x04, 0x04); // Tape output latch byte 2
	map(0x05, 0x05); // Tape output latch byte 1
	map(0x07, 0x07); // Indicator load (LED)
	map(0x10, 0x13).rw("sio1", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 1 for user 1 & 2
	map(0x18, 0x1b).rw("sio5", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 5 for user 9 & 10
	map(0x20, 0x23).rw("sio2", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 2 for user 3 & 4
	map(0x28, 0x2b).rw("sio6", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 6 for user 11 & 12
	map(0x30, 0x33).rw("sio3", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 3 for user 5 & 6
	map(0x38, 0x3b).rw("sio7", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 7 for user 13 & 14
	map(0x40, 0x43).rw("sio4", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 4 for user 7 & 8
	map(0x48, 0x4b).rw("sio8", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 8 for user 15 & 16
	//AM_RANGE(0x50, 0x53) // SIO 0 for RS232 1 and part of tape interface
	map(0x50, 0x50).r(FUNC(ts816_state::keyin_r)).w(m_terminal, FUNC(generic_terminal_device::write));
	map(0x52, 0x52).r(FUNC(ts816_state::status_r));
	map(0x58, 0x5b).rw("sio9", FUNC(z80sio_device::cd_ba_r), FUNC(z80sio_device::cd_ba_w)); // SIO 9 for RS232 2 & 3
	map(0x60, 0x60).portr("DSW");
	map(0x68, 0x68).w(FUNC(ts816_state::port68_w)); // set 2nd bank latch
	map(0x70, 0x78).w(FUNC(ts816_state::port78_w)); // reset 2nd bank latch (manual can't decide between 70 and 78, so we take both)
	map(0x80, 0x83).rw("ctc1", FUNC(z80ctc_device::read), FUNC(z80ctc_device::write)); // CTC 1 (ch 0 baud A)
	map(0x90, 0x93).rw("dma", FUNC(z80dma_device::bus_r), FUNC(z80dma_device::bus_w)); // DMA
	map(0xA0, 0xA0); // WDC status / command
	map(0xA1, 0xA1); // WDC data
	map(0xB0, 0xB0).noprw(); // undocumented, written to at @0707 and @0710
	map(0xC0, 0xC3).rw("ctc2", FUNC(z80ctc_device::read), FUNC(z80ctc_device::write)); // CTC 2 (ch 0 baud B, ch 1 baud C)
	map(0xD0, 0xD3).rw("pio", FUNC(z80pio_device::read), FUNC(z80pio_device::write));
	map(0xE0, 0xE0).w(FUNC(ts816_state::porte0_w)); // set ENDRAM memory banking
	map(0xF0, 0xF0).w(FUNC(ts816_state::portf0_w)); // reset ENDRAM memory banking
}


/* Input ports */
static INPUT_PORTS_START( ts816 )
	PORT_START("DSW") //
	PORT_DIPNAME( 0x07, 0x01, "System Terminal") // read at @0368
	PORT_DIPSETTING(    0x00, "19200 baud")
	PORT_DIPSETTING(    0x01, "9600 baud")
	PORT_DIPSETTING(    0x02, "4800 baud")
	PORT_DIPSETTING(    0x03, "2400 baud")
	PORT_DIPSETTING(    0x04, "1200 baud")
	PORT_DIPSETTING(    0x05, "600 baud")
	PORT_DIPSETTING(    0x06, "300 baud")
	PORT_DIPSETTING(    0x07, "150 baud")
	PORT_DIPNAME( 0x80, 0x00, "Operation Switch") // this switch checked @006F (undocumented)
	PORT_DIPSETTING(    0x80, DEF_STR(On))
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END


READ8_MEMBER( ts816_state::keyin_r )
{
	uint8_t ret = m_term_data;
	m_term_data = 0;
	return ret;
}

READ8_MEMBER( ts816_state::status_r )
{
	if (m_status)
	{
		m_status--;
		return 5;
	}
	else
		return 4;
}

WRITE8_MEMBER( ts816_state::port68_w )
{
	m_2ndbank = 1;
	set_banks();
}

WRITE8_MEMBER( ts816_state::port78_w )
{
	m_2ndbank = 0;
	set_banks();
}

WRITE8_MEMBER( ts816_state::porte0_w )
{
	m_endram = 1;
	set_banks();
}

WRITE8_MEMBER( ts816_state::portf0_w )
{
	m_endram = 0;
	set_banks();
}

void ts816_state::set_banks()
{
	if (!m_2ndbank)
	{
		if (!m_endram)
		{
			// bootup setting
			membank("bankr0")->set_entry(2); // point at rom
			membank("bankw0")->set_entry(0);
			membank("bank1")->set_entry(0);
			membank("bank2")->set_entry(0);
		}
		else
		{
			// 64k ram (lower half)
			membank("bankr0")->set_entry(0);
			membank("bankw0")->set_entry(0);
			membank("bank1")->set_entry(0);
			membank("bank2")->set_entry(0);
		}
	}
	else
	{
		if (!m_endram)
		{
			// not documented, so assuming roms with ram (upper half)
			membank("bankr0")->set_entry(2);
			membank("bankw0")->set_entry(1);
			membank("bank1")->set_entry(1);
			membank("bank2")->set_entry(1);
		}
		else
		{
			// split of upper and lower ram (not documented if ram has a new address, assuming not)
			membank("bankr0")->set_entry(1);
			membank("bankw0")->set_entry(1);
			membank("bank1")->set_entry(1);
			membank("bank2")->set_entry(0);
		}
	}
}

void ts816_state::kbd_put(u8 data)
{
	m_term_data = data;
	m_status = 3;
}

void ts816_state::machine_reset()
{
	m_2ndbank = 0;
	m_endram = 0;
	m_term_data = 0;
	m_status = 1;
	set_banks();
	m_maincpu->reset();
}

// correct order yet to be determined
static const z80_daisy_config daisy_chain[] =
{
	{ "dma" },
	{ "pio" },
	{ "ctc1" },
	{ "ctc2" },
//  { "sio0" },
	{ "sio1" },
	{ "sio2" },
	{ "sio3" },
	{ "sio4" },
	{ "sio5" },
	{ "sio6" },
	{ "sio7" },
	{ "sio8" },
	{ "sio9" },
	{ nullptr }
};

void ts816_state::init_ts816()
{
	uint8_t *roms = memregion("roms")->base();
	uint8_t *rams = memregion("rams")->base();

	// 0000-3FFF
	membank("bankr0")->configure_entry(2, &roms[0x00000]); // roms
	membank("bankr0")->configure_entry(0, &rams[0x00000]);
	membank("bankr0")->configure_entry(1, &rams[0x10000]);
	membank("bankw0")->configure_entry(0, &rams[0x00000]);
	membank("bankw0")->configure_entry(1, &rams[0x10000]);
	// 4000-DFFF
	membank("bank1")->configure_entry(0, &rams[0x04000]);
	membank("bank1")->configure_entry(1, &rams[0x14000]);
	// E000-FFFF
	membank("bank2")->configure_entry(0, &rams[0x0e000]);
	membank("bank2")->configure_entry(1, &rams[0x1e000]);
}

MACHINE_CONFIG_START(ts816_state::ts816)
	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", Z80, XTAL(16'000'000) / 4)
	MCFG_DEVICE_PROGRAM_MAP(ts816_mem)
	MCFG_DEVICE_IO_MAP(ts816_io)
	MCFG_Z80_DAISY_CHAIN(daisy_chain)

	/* video hardware */
	MCFG_DEVICE_ADD("terminal", GENERIC_TERMINAL, 0)
	MCFG_GENERIC_TERMINAL_KEYBOARD_CB(PUT(ts816_state, kbd_put))

	//MCFG_DEVICE_ADD("sio0", Z80SIO, XTAL(16'000'000) / 4)
	//MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio1", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio2", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio3", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio4", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio5", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio6", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio7", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio8", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("sio9", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))

	MCFG_DEVICE_ADD("pio", Z80PIO, XTAL(16'000'000) / 4)
	MCFG_Z80PIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	//MCFG_Z80PIO_IN_PA_CB(READ8(*this, ts816_state, porta_r))
	//MCFG_Z80PIO_IN_PB_CB(READ8(*this, ts816_state, portb_r))
	//MCFG_Z80PIO_OUT_PB_CB(WRITE8(*this, ts816_state, portb_w))

	MCFG_DEVICE_ADD("ctc1", Z80CTC, XTAL(16'000'000) / 4)
	MCFG_Z80CTC_INTR_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_DEVICE_ADD("ctc2", Z80CTC, XTAL(16'000'000) / 4)
	MCFG_Z80CTC_INTR_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))

	MCFG_DEVICE_ADD("dma", Z80DMA, XTAL(16'000'000) / 4)
	//MCFG_Z80DMA_OUT_BUSREQ_CB(WRITELINE(*this, ts816_state, busreq_w))
	MCFG_Z80DMA_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ts816 )
	ROM_REGION(0x4000, "roms", 0)
	ROM_LOAD( "81640v11.rom", 0x0000, 0x1000, BAD_DUMP CRC(295a15e7) SHA1(6f49078ab3cd49aecd2afafcbed3af0e3bcfd48c) ) // both halves identical

	ROM_REGION(0x20000, "rams", ROMREGION_ERASEFF)
ROM_END

/* Driver */

//    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  STATE        INIT        COMPANY      FULLNAME  FLAGS
COMP( 1980, ts816, 0,      0,      ts816,   ts816, ts816_state, init_ts816, "Televideo", "TS816",  MACHINE_IS_SKELETON )
