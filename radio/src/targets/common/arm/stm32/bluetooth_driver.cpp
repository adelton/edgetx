/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "bluetooth_driver.h"
#include "stm32_hal_ll.h"

#include "board.h"
#include "debug.h"

#if !defined(BOOT)

#include "stm32_serial_driver.h"

#define BT_USART_IRQ_PRIORITY 6

static const stm32_usart_t btUSART = {
  .USARTx = BT_USART,
  .GPIOx = BT_USART_GPIO,
  .GPIO_Pin = BT_TX_GPIO_PIN | BT_RX_GPIO_PIN,
  .IRQn = BT_USART_IRQn,
  .IRQ_Prio = BT_USART_IRQ_PRIORITY,
  .txDMA = nullptr,
  .txDMA_Stream = 0,
  .txDMA_Channel = 0,
  .rxDMA = nullptr,
  .rxDMA_Stream = 0,
  .rxDMA_Channel = 0,
};

DEFINE_STM32_SERIAL_PORT(BTModule, btUSART, BT_RX_FIFO_SIZE, BT_TX_FIFO_SIZE);

#if defined(BLUETOOTH_PROBE)
volatile uint8_t btChipPresent = 0;
#endif

// const etx_serial_driver_t* _bt_usart_drv = nullptr;
void* _bt_usart_ctx = nullptr;
#endif

void bluetoothInit(uint32_t baudrate, bool enable)
{
  LL_GPIO_InitTypeDef pinInit;
  LL_GPIO_StructInit(&pinInit);

  pinInit.Pin = BT_EN_GPIO_PIN;
  pinInit.Mode = LL_GPIO_MODE_OUTPUT;
  pinInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  pinInit.Pull = LL_GPIO_PULL_NO;

  LL_GPIO_Init(BT_EN_GPIO, &pinInit);

#if !defined(BOOT)
  etx_serial_init cfg = {
    .baudrate = baudrate,
    .encoding = ETX_Encoding_8N1,
    .direction = ETX_Dir_TX_RX,
    .polarity = ETX_Pol_Normal,
  };

  if (!_bt_usart_ctx) {
    auto hw_def = REF_STM32_SERIAL_PORT(BTModule);
    _bt_usart_ctx = STM32SerialDriver.init(hw_def, &cfg);
  } else {
    STM32SerialDriver.setBaudrate(_bt_usart_ctx, baudrate);
  }
#endif

  if (enable) {
    LL_GPIO_ResetOutputPin(BT_EN_GPIO, BT_EN_GPIO_PIN);
  } else {
    LL_GPIO_SetOutputPin(BT_EN_GPIO, BT_EN_GPIO_PIN);
  }
}

#if !defined(BOOT)
void bluetoothDisable()
{
  // close bluetooth (recent modules will go to bootloader mode)
  LL_GPIO_SetOutputPin(BT_EN_GPIO, BT_EN_GPIO_PIN);
  if (_bt_usart_ctx) {
    STM32SerialDriver.deinit(_bt_usart_ctx);
  }
}

// @Cliff
// ISR is handled by the serial driver,
// however, it is unclear at this point
// how the detection should be implemented.
// Maybe setting a temporary callback on
// the interface, or trying to pull some data.
//
// Also, it is quite unclear why we would need
// to disable sending when the TX buffer is exahausted.
// The serial driver does take care of that but does not
// set any particular piece of state that could be queried
// to deduct the state of this TX buffer.
#if _OBSOLETE_
extern "C" void BT_USART_IRQHandler(void)
{
  DEBUG_INTERRUPT(INT_BLUETOOTH);
  if (USART_GetITStatus(BT_USART, USART_IT_RXNE) != RESET) {
    USART_ClearITPendingBit(BT_USART, USART_IT_RXNE);
    uint8_t byte = USART_ReceiveData(BT_USART);
    btRxFifo.push(byte);
#if defined(BLUETOOTH_PROBE)
    if (!btChipPresent) {
      // This is to differentiate X7 and X7S and X-Lite with/without BT
      btChipPresent = 1;
      bluetoothDisable();
    }
#endif
  }

  if (USART_GetITStatus(BT_USART, USART_IT_TXE) != RESET) {
    uint8_t byte;
    bool result = btTxFifo.pop(byte);
    if (result) {
      USART_SendData(BT_USART, byte);
    }
    else {
      USART_ITConfig(BT_USART, USART_IT_TXE, DISABLE);
      // => BLUETOOTH_WRITE_IDLE
      bluetoothWriteState = BLUETOOTH_WRITE_DONE;
    }
  }
}
#endif

void bluetoothWrite(const void* buffer, uint32_t len)
{
  if (!_bt_usart_ctx) return;
  STM32SerialDriver.sendBuffer(_bt_usart_ctx, (const uint8_t*)buffer, len);
}

int bluetoothRead(uint8_t* data)
{
  if (!_bt_usart_ctx) return 0;
  return STM32SerialDriver.getByte(_bt_usart_ctx, data);
}

uint8_t bluetoothIsWriting(void)
{
  if (!_bt_usart_ctx)
    return false;
  
  return STM32SerialDriver.txCompleted(_bt_usart_ctx);
}
#endif // !BOOT
