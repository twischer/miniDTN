//--------------------------------------------------------------
// File     : stm32_ub_spi2.c
// Datum    : 04.03.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : GPIO, SPI
// Funktion : SPI-LoLevel-Funktionen (SPI-2)
//
// Hinweis  : mögliche Pinbelegungen
//            SPI2 : SCK :[PB10, PB13] 
//                   MOSI:[PB15, PC3]
//                   MISO:[PB14, PC2]
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_spi2.h"


//--------------------------------------------------------------
// Definition von SPI2
//--------------------------------------------------------------
SPI2_DEV_t SPI2DEV = {
// PORT , PIN       , Clock              , Source 
  .SCK = {GPIOB,GPIO_Pin_10,RCC_AHB1Periph_GPIOB,GPIO_PinSource10}, // SCK an PB10
  .MOSI = {GPIOC,GPIO_Pin_3,RCC_AHB1Periph_GPIOC,GPIO_PinSource3}, // MOSI an PC3
  .MISO = {GPIOC,GPIO_Pin_2,RCC_AHB1Periph_GPIOC,GPIO_PinSource2}, // MISO an PC2
};



//--------------------------------------------------------------
// Init von SPI2
// Return_wert :
//  -> ERROR   , wenn SPI schon mit anderem Mode initialisiert
//  -> SUCCESS , wenn SPI init ok war
//--------------------------------------------------------------
ErrorStatus UB_SPI2_Init(SPI2_Mode_t mode)
{
  ErrorStatus ret_wert=ERROR;
  static uint8_t init_ok=0;
  static SPI2_Mode_t init_mode;
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

  // initialisierung darf nur einmal gemacht werden
  if(init_ok!=0) {
    if(init_mode==mode) ret_wert=SUCCESS;
    return(ret_wert);
  }

  // SPI-Clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);  

  // Clock Enable der Pins
  RCC_AHB1PeriphClockCmd(SPI2DEV.SCK.CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(SPI2DEV.MOSI.CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(SPI2DEV.MISO.CLK, ENABLE);

  // SPI Alternative-Funktions mit den IO-Pins verbinden
  GPIO_PinAFConfig(SPI2DEV.SCK.PORT, SPI2DEV.SCK.SOURCE, GPIO_AF_SPI2);
  GPIO_PinAFConfig(SPI2DEV.MOSI.PORT, SPI2DEV.MOSI.SOURCE, GPIO_AF_SPI2);
  GPIO_PinAFConfig(SPI2DEV.MISO.PORT, SPI2DEV.MISO.SOURCE, GPIO_AF_SPI2);

  // SPI als Alternative-Funktion mit PullDown
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

  // SCK-Pin
  GPIO_InitStructure.GPIO_Pin = SPI2DEV.SCK.PIN;
  GPIO_Init(SPI2DEV.SCK.PORT, &GPIO_InitStructure);
  // MOSI-Pin
  GPIO_InitStructure.GPIO_Pin = SPI2DEV.MOSI.PIN;
  GPIO_Init(SPI2DEV.MOSI.PORT, &GPIO_InitStructure);
  // MISO-Pin
  GPIO_InitStructure.GPIO_Pin = SPI2DEV.MISO.PIN;
  GPIO_Init(SPI2DEV.MISO.PORT, &GPIO_InitStructure);

  // SPI-Konfiguration
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  if(mode==SPI_MODE_0) {
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  }else if(mode==SPI_MODE_1) {
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  }else if(mode==SPI_MODE_2) {
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  }else {
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  }
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI2_VORTEILER;

  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI2, &SPI_InitStructure); 

  // SPI enable
  SPI_Cmd(SPI2, ENABLE); 

  // init Mode speichern
  init_ok=1;
  init_mode=mode;
  ret_wert=SUCCESS;

  return(ret_wert);
}


//--------------------------------------------------------------
// sendet und empfängt ein Byte per SPI2
// ChipSelect-Signal muss von rufender Funktion gemacht werden
//--------------------------------------------------------------
uint8_t UB_SPI2_SendByte(uint8_t wert)
{ 
  uint8_t ret_wert=0;

  // warte bis senden fertig
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);

  // byte senden
  SPI_I2S_SendData(SPI2, wert);

  // warte bis empfang fertig
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET); 

  // Daten einlesen
  ret_wert=SPI_I2S_ReceiveData(SPI2);

  return(ret_wert);
}
