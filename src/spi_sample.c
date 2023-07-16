#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sample);

#define STACK_SIZE 2048
#define PRIORITY K_LOWEST_THREAD_PRIO - 1

#define SPI1_NODE DT_NODELABEL(spi1)
const struct device *const spi1 = DEVICE_DT_GET(SPI1_NODE);
struct spi_config spi1_config = {
	.frequency = 1000000,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(16),
	.slave = 0,
	.cs = {
		.gpio = GPIO_DT_SPEC_GET(SPI1_NODE, cs_gpios),
		.delay = 0u,
	},
};

int xfer_spi(const struct device *dev, struct spi_config *config,
				uint16_t *tx_buf, uint16_t *rx_buf, uint32_t xfer_size)
{
	struct spi_buf tx_data[1] = {
		{.buf = tx_buf, .len = xfer_size},
	};
	struct spi_buf rx_data[1] = {
		{.buf = rx_buf, .len = xfer_size},
	};

	struct spi_buf_set tx_set = { .buffers = tx_data, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_data, .count = 1 };

	return spi_transceive(dev, config, &tx_set, &rx_set);
}

// https://openimu.readthedocs.io/en/latest/software/SPImessaging.html
// 3. OpenIMU SPI Register Read Methodology

// SPI master initiates a register read (for example register 0x04) by clocking in the address followed
// by 0x00, i.e. 0x0400, via MOSI. This combination is referred to as a read-command. It is followed by 1
// 6 zero-bits to complete the SPI data-transfer cycle. As the master transmits the read command
// over MOSI, the OpenIMU transmits information back over MISO.

// // In this transmission, the first data-word sent by the OpenIMU (as the read-command is sent)
// consists of 16-bits of non-applicable data. The subsequent 16-bit message contains information
// stored inside two consecutive registers (in this case registers 4 (MSB) and 5(LSB)).
void test_OpenIMU_SPI_Register_Read_Methodology()
{
	enum {SizeOfTransfer = 2};
	uint16_t tx[SizeOfTransfer] = {0x0400, 0x0000};
	uint16_t rx[SizeOfTransfer] = {0};
	int ret = xfer_spi(spi1, &spi1_config, tx, rx, sizeof(tx));
	if (ret < 0) {
		LOG_WRN("Fail spi xfer ret: %d", ret);
		return;
	}

	LOG_INF("rx %04x", rx[1]);
}


void spi_sample_entry(void *p1, void *p2, void *p3)
{
	UNUSED(p1); UNUSED(p2); UNUSED(p3);


	while(1) {
		test_OpenIMU_SPI_Register_Read_Methodology();
		k_msleep(1000);
	}

}

K_THREAD_DEFINE(spi_sample_task, STACK_SIZE, spi_sample_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);