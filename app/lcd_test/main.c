#include "rtenv.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"

int main() {
  uint16_t linenum = 0;
  static TP_STATE* TP_State; 
	LCD_Init();
	LCD_LayerInit();
	LTDC_Cmd(ENABLE);
	LCD_SetLayer(LCD_FOREGROUND_LAYER);
  LCD_Clear(LCD_COLOR_WHITE);
//	LCD_DrawRect(40,40,40,40);
IOE_Config();
    LCD_SetFont(&Font8x8);
    LCD_DisplayStringLine(LINE(32), (uint8_t*)"              Touch Panel Paint     ");
    LCD_DisplayStringLine(LINE(34), (uint8_t*)"              Example               ");
    LCD_SetTextColor(LCD_COLOR_BLUE2); 
    LCD_DrawFullRect(5, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_CYAN); 
    LCD_DrawFullRect(40, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_YELLOW); 
    LCD_DrawFullRect(75, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_RED); 
    LCD_DrawFullRect(5, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLUE); 
    LCD_DrawFullRect(40, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_GREEN); 
    LCD_DrawFullRect(75, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_MAGENTA); 
    LCD_DrawFullRect(145, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DrawFullRect(110, 288, 30, 30);
    LCD_DrawRect(180, 270, 48, 50);
    LCD_SetFont(&Font16x24);
    LCD_DisplayChar(LCD_LINE_12, 195, 0x43);
    LCD_DrawLine(0, 248, 240, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(0, 284, 180, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(1, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(37, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(72, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(107, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(142, 284, 36, LCD_DIR_VERTICAL);
    LCD_DrawLine(0, 319, 240, LCD_DIR_HORIZONTAL);
  while (1)
  {
 
    TP_State = IOE_TP_GetState();
    
    if((TP_State->TouchDetected) && ((TP_State->Y < 245) && (TP_State->Y >= 3)))
    {
      if((TP_State->X >= 237) || (TP_State->X < 3))
      {}     
      else
      {
        LCD_DrawFullCircle(TP_State->X, TP_State->Y, 3);
      }
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 280) && (TP_State->Y >= 250) && (TP_State->X >= 5) && (TP_State->X <= 35))
    {
      LCD_SetTextColor(LCD_COLOR_BLUE2);
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 280) && (TP_State->Y >= 250) && (TP_State->X >= 40) && (TP_State->X <= 70))
    {
      LCD_SetTextColor(LCD_COLOR_CYAN); 
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 280) && (TP_State->Y >= 250) && (TP_State->X >= 75) && (TP_State->X <= 105))
    {
      LCD_SetTextColor(LCD_COLOR_YELLOW); 
    }      
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 288) && (TP_State->X >= 5) && (TP_State->X <= 35))
    {
      LCD_SetTextColor(LCD_COLOR_RED);
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 288) && (TP_State->X >= 40) && (TP_State->X <= 70))
    {
      LCD_SetTextColor(LCD_COLOR_BLUE); 
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 288) && (TP_State->X >= 75) && (TP_State->X <= 105))
    {
      LCD_SetTextColor(LCD_COLOR_GREEN); 
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 288) && (TP_State->X >= 110) && (TP_State->X <= 140))
    {
      LCD_SetTextColor(LCD_COLOR_BLACK); 
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 288) && (TP_State->X >= 145) && (TP_State->X <= 175))
    {
      LCD_SetTextColor(LCD_COLOR_MAGENTA); 
    }
    else if ((TP_State->TouchDetected) && (TP_State->Y <= 318) && (TP_State->Y >= 270) && (TP_State->X >= 180) && (TP_State->X <= 230))
    {
      LCD_SetFont(&Font8x8);
      for(linenum = 0; linenum < 31; linenum++)
      {
        LCD_ClearLine(LINE(linenum));
      }
    }
    else
    {
    }
  }
	return 0;
}
