#ifndef POC_SX1262_SX1262_H_
#define POC_SX1262_SX1262_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t chip_mode;
  uint8_t cmd_status;
} sx1262_chip_status_t;

void sx1262_chip_config(void);
void sx1262_radio_config(void);
bool sx1262_chip_status(sx1262_chip_status_t *out_status);
void sx1262_send_payload(uint8_t *payload, uint8_t length);
void sx1262_set_rx(void);
void sx1262_set_tx(void);

#endif /* POC_SX1262_SX1262_H_ */
