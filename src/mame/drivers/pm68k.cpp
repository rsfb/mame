// license:BSD-3-Clause
// copyright-holders:Robbbert
/***************************************************************************

Callan PM68K Unix mainframe.

2013-09-04 Skeleton driver

Status: Boots into monitor, some commands work, some freeze.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "bus/rs232/rs232.h"
#include "machine/am9513.h"
#include "machine/z80sio.h"


class pm68k_state : public driver_device
{
public:
	pm68k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_p_base(*this, "rambase")
		, m_maincpu(*this, "maincpu")
	{ }

	void pm68k(machine_config &config);
	void pm68k_mem(address_map &map);
private:
	virtual void machine_reset() override;
	required_shared_ptr<uint16_t> m_p_base;
	required_device<cpu_device> m_maincpu;
};


void pm68k_state::pm68k_mem(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xffffff);
	map(0x000000, 0x1fffff).ram().share("rambase");
	map(0x200000, 0x205fff).rom().region("roms", 0);
	map(0x600000, 0x600007).rw("mpsc", FUNC(i8274_new_device::ba_cd_r), FUNC(i8274_new_device::ba_cd_w)).umask16(0xff00);
	map(0x800000, 0x800003).rw("stc", FUNC(am9513_device::read16), FUNC(am9513_device::write16));
}


/* Input ports */
static INPUT_PORTS_START( pm68k )
INPUT_PORTS_END


void pm68k_state::machine_reset()
{
	uint8_t* ROM = memregion("roms")->base();
	memcpy(m_p_base, ROM, 8);
	m_maincpu->reset();
}

MACHINE_CONFIG_START(pm68k_state::pm68k)
	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", M68000, 8000000)
	MCFG_DEVICE_PROGRAM_MAP(pm68k_mem)

	MCFG_DEVICE_ADD("mpsc", I8274_NEW, 0)
	MCFG_Z80SIO_OUT_TXDA_CB(WRITELINE("rs232a", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRA_CB(WRITELINE("rs232a", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSA_CB(WRITELINE("rs232a", rs232_port_device, write_rts))
	MCFG_Z80SIO_OUT_TXDB_CB(WRITELINE("rs232b", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRB_CB(WRITELINE("rs232b", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSB_CB(WRITELINE("rs232b", rs232_port_device, write_rts))

	MCFG_DEVICE_ADD("stc", AM9513, 4000000)
	MCFG_AM9513_OUT4_CALLBACK(WRITELINE("mpsc", i8274_new_device, rxca_w))
	MCFG_DEVCB_CHAIN_OUTPUT(WRITELINE("mpsc", i8274_new_device, txca_w))
	MCFG_AM9513_OUT5_CALLBACK(WRITELINE("mpsc", i8274_new_device, rxcb_w))
	MCFG_DEVCB_CHAIN_OUTPUT(WRITELINE("mpsc", i8274_new_device, txcb_w))

	MCFG_DEVICE_ADD("rs232a", RS232_PORT, default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(WRITELINE("mpsc", i8274_new_device, rxa_w))
	MCFG_RS232_DSR_HANDLER(WRITELINE("mpsc", i8274_new_device, dcda_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE("mpsc", i8274_new_device, ctsa_w))

	MCFG_DEVICE_ADD("rs232b", RS232_PORT, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(WRITELINE("mpsc", i8274_new_device, rxb_w))
	MCFG_RS232_DSR_HANDLER(WRITELINE("mpsc", i8274_new_device, dcdb_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE("mpsc", i8274_new_device, ctsb_w))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pm68k )
	ROM_REGION16_BE(0x6000, "roms", 0)
	ROM_LOAD16_BYTE("u103", 0x00000, 0x1000, CRC(86d32d6c) SHA1(ce9c54b62c64c37ae9106fb06b8a2b2152d1ddf6) )
	ROM_LOAD16_BYTE("u101", 0x00001, 0x1000, CRC(66607e54) SHA1(06f380fdeba13dc3aee826dd166f4bd3031febb9) )
	ROM_LOAD16_BYTE("u104", 0x02000, 0x2000, CRC(ccd2ba4d) SHA1(5cdcf875e136aa9af5f150e0102cd209c496885e) )
	ROM_LOAD16_BYTE("u102", 0x02001, 0x2000, CRC(48182abd) SHA1(a6e4fb62c5f04cb397c6c3294723ec1f7bc3b680) )
ROM_END


/* Driver */

//    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT  CLASS        INIT        COMPANY                FULLNAME  FLAGS
COMP( 198?, pm68k, 0,      0,      pm68k,  pm68k, pm68k_state, empty_init, "Callan Data Systems", "PM68K",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
