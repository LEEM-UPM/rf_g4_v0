# rf_g4_v0

Firmware de la placa de telemetría RF para el cohete de sondeo universitario LEEM (UPM).

## El sistema
 
La aviónica del cohete está compuesta por cuatro placas independientes que se comunican mediante un bus CAN:
 
| Placa | Responsabilidad |
|---|---|
| **RF** (este repositorio) | Telemetría RF en tiempo real a estación de tierra |
| **Core** | Cerebro del sistema, almacenamiento en µSD, toma de decisiones |
| **Sensórica** | IMU, GNSS, temperatura, humedad, giróscopo |
| **Power** | Gestión de alimentación, potencia y pirotécnicos |

### Hardware de la placa RF
 
- **MCU:** STM32G473CET6 (Cortex-M4 @ 170 MHz, 512 KB Flash, 128 KB RAM)
- **Radio sub-GHz:** Semtech SX1262 — LoRa 868 MHz, hasta +22 dBm
- **Radio 2.4 GHz:** Semtech SX1280 — LoRa/FLRC/BLE
- **Bus:** FDCAN × 2 (nodo de la red CAN del cohete)
- **Toolchain:** arm-none-eabi-gcc · STM32CubeMX · CMake · Ninja