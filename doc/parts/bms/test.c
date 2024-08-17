static u8 bq40z50_regs[NUM_REGS] = {
	0x00,	/* CONTROL */
	0x08,	/* TEMP */
	0x09,	/* VOLT */
	0x0B,	/* AVG CURRENT */
	0x16,	/* FLAGS */
	0x12,	/* Time to empty */
	0x13,	/* Time to full */
	0x10,	/* Full charge capacity */
	0x0F,	/* Remaining Capacity */
	0x17,	/* CycleCount */
	0x0D,	/* State of Charge */
	0x4F,	/* State of Health */
	0x18,	/* Design Capacity */
	0x44,	/* ManufacturerBlockAccess*/
};

enum bq_fg_mac_cmd {
	FG_MAC_CMD_OP_STATUS	= 0x0000,
	FG_MAC_CMD_DEV_TYPE	= 0x0001,
	FG_MAC_CMD_FW_VER	= 0x0002,
	FG_MAC_CMD_HW_VER	= 0x0003,
	FG_MAC_CMD_IF_SIG	= 0x0004,
	FG_MAC_CMD_CHEM_ID	= 0x0006,
	FG_MAC_CMD_GAUGING	= 0x0021,
	FG_MAC_CMD_SEAL		= 0x0030,
	FG_MAC_CMD_DEV_RESET	= 0x0041,
};

static int fg_mac_read_block(struct bq_fg_chip *bq, u16 cmd, u8 *buf, u8 len)
{
	int ret;
	u8 t_buf[40];
	u8 t_len;
	int i;

	t_buf[0] = (u8)(cmd >> 8);
	t_buf[1] = (u8)cmd;
	ret = fg_write_block(bq, bq->regs[BQ_FG_REG_MBA], t_buf, 2);
	if (ret < 0)
		return ret;

	msleep(100);

	ret = fg_read_block(bq, bq->regs[BQ_FG_REG_MBA], t_buf, 36);
	if (ret < 0)
		return ret;
	t_len = ret;
	/* ret contains number of data bytes in gauge's response*/
	fg_print_buf("mac_read_block", t_buf, t_len);

	for (i = 0; i < t_len - 2; i++)
		buf[i] = t_buf[i+2];

	return 0;
}

static void fg_read_fw_version(struct bq_fg_chip *bq)
{

	int ret;
	u8 buf[36];

	ret = fg_write_word(bq, bq->regs[BQ_FG_REG_MBA], FG_MAC_CMD_FW_VER);

	if (ret < 0) {
		bq_err("Failed to send firmware version subcommand:%d\n", ret);
		return;
	}

	mdelay(2);

	ret = fg_mac_read_block(bq, bq->regs[BQ_FG_REG_MBA], buf, 11);
	if (ret < 0) {
		bq_err("Failed to read firmware version:%d\n", ret);
		return;
	}

	bq_log("FW Ver:%04X, Build:%04X\n",
		buf[2] << 8 | buf[3], buf[4] << 8 | buf[5]);
	bq_log("Ztrack Ver:%04X\n", buf[7] << 8 | buf[8]);
}