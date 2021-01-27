#include "stm32f0xx.h"
#include "core_cm0.h" 

enum {RELAY_OFF, RELAY_ON};
      
#define _IN0 PA0        //in    exti0
#define _IN1 PA1        //in    exti1
#define _IN0_INT PA2    //out
#define _IN1_INT PA3    //out
#define _R_RELAY0 PA4   //in    exti4
#define _S_RELAY0 PA5   //in    exti5
#define _R_RELAY1 PA6   //in    exti6
#define _S_RELAY1 PA7   //in    exti7
#define _RELAY_0 PF0    //out
#define _RELAY_1 PF1    //out

#define _BUTTON PB1     //in

#define TX32 PA9
#define RX32 PA10

volatile uint8_t tmr_in0_debounce=0, tmr_in1_debounce=0;
volatile uint32_t tmr_in0_delay=0xff, tmr_in0_pulse=0;
volatile uint32_t tmr_in1_delay=0xff, tmr_in1_pulse=0;

#define BUILD_MAJOR 0
#define BUILD_MINOR 2
#define VERSION STRINGIZE(BUILD_MAJOR) "." STRINGIZE(BUILD_MINOR)

#define TIME_DEBOUNCE_MS 50
#define TIME_PULSE_IN_MS    100//51//149
#define TIME_CHANGE_MODE_MS 500//1000

uint8_t state_relay_0=RELAY_OFF, state_relay_1=RELAY_OFF;

void EXTI0_1_IRQHandler(void) {
//#define _IN0 PA0        //in    exti0
  if(EXTI->PR & EXTI_PR_PR0) {
    EXTI->PR = EXTI_PR_PR0;
    
    if(GPIOA->IDR & GPIO_IDR_0) {      
      tmr_in0_debounce = TIME_DEBOUNCE_MS;      
    } else {
      tmr_in0_debounce = TIME_DEBOUNCE_MS;      //press        
    }
  }
//  #define _IN1 PA1        //in    exti1
  if(EXTI->PR & EXTI_PR_PR1) {
    EXTI->PR = EXTI_PR_PR1;
    
    if(GPIOA->IDR & GPIO_IDR_1) {
      tmr_in1_debounce = TIME_DEBOUNCE_MS;  
    } else {
      tmr_in1_debounce = TIME_DEBOUNCE_MS;  
    }
  }   
}

void EXTI4_15_IRQHandler(void) {
//#define _R_RELAY0 PA4   //in    exti4
  if(EXTI->PR & EXTI_PR_PR4) {
    EXTI->PR = EXTI_PR_PR4;
    
    if(GPIOA->IDR & GPIO_IDR_4) {
      GPIOF->ODR &= ~GPIO_ODR_0;   
      state_relay_0 = RELAY_OFF;
    }     
  }
//#define _S_RELAY0 PA5   //in    exti5  
  if(EXTI->PR & EXTI_PR_PR5) {
    EXTI->PR = EXTI_PR_PR5;
    
    if(GPIOA->IDR & GPIO_IDR_5) {
      if(!(GPIOA->IDR & GPIO_IDR_4)) {
        GPIOF->ODR |= GPIO_ODR_0; 
        state_relay_0 = RELAY_ON;
      }
    }       
  }  
//#define _R_RELAY1 PA6   //in    exti6  
  if(EXTI->PR & EXTI_PR_PR6) {
    EXTI->PR = EXTI_PR_PR6;
    
    if(GPIOA->IDR & GPIO_IDR_6) {
      GPIOF->ODR &= ~GPIO_ODR_1;   
      state_relay_1 = RELAY_OFF;
    }
  }
//#define _S_RELAY1 PA7   //in    exti7  
  if(EXTI->PR & EXTI_PR_PR7) {
    EXTI->PR = EXTI_PR_PR7;
    
    if(GPIOA->IDR & GPIO_IDR_7) {
      if(!(GPIOA->IDR & GPIO_IDR_6)) {
        GPIOF->ODR |= GPIO_ODR_1;
        state_relay_1 = RELAY_ON;
      }        
    }
  }  
  
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {
  if(TIM1->SR & TIM_SR_UIF) {
    TIM1->SR &= ~TIM_SR_UIF;    
    TIM1->CNT = 57535;
    
    //in0
    if(tmr_in0_debounce) {
      tmr_in0_debounce--;
      if(!tmr_in0_debounce) {
        if(GPIOA->IDR & GPIO_IDR_0) {
          if(tmr_in0_delay > TIME_CHANGE_MODE_MS) {
            if(state_relay_0 == RELAY_ON) {
              GPIOA->ODR &= ~GPIO_ODR_2;
              tmr_in0_pulse = TIME_PULSE_IN_MS;
            }
          }          
        } else {
          GPIOA->ODR &= ~GPIO_ODR_2;
          tmr_in0_pulse = TIME_PULSE_IN_MS;
          tmr_in0_delay = 0;
        }        
      }
    }
    
    if(tmr_in0_pulse) {
      tmr_in0_pulse--;
      if(!tmr_in0_pulse) {
        GPIOA->ODR |= GPIO_ODR_2;
      }
    }
    
    if(!(GPIOA->IDR & GPIO_IDR_0)) {
      if(tmr_in0_delay < 10000) {
        tmr_in0_delay++;
      }
    }   
    
    //in1
    if(tmr_in1_debounce) {
      tmr_in1_debounce--;
      if(!tmr_in1_debounce) {
        if(GPIOA->IDR & GPIO_IDR_1) {
          if(tmr_in1_delay > TIME_CHANGE_MODE_MS) {
            if(state_relay_1 == RELAY_ON) {
              GPIOA->ODR &= ~GPIO_ODR_3;
              tmr_in1_pulse = TIME_PULSE_IN_MS;
            }
          }          
        } else {
          GPIOA->ODR &= ~GPIO_ODR_3;
          tmr_in1_pulse = TIME_PULSE_IN_MS;
          tmr_in1_delay = 0;
        }        
      }
    }
    
    if(tmr_in1_pulse) {
      tmr_in1_pulse--;
      if(!tmr_in1_pulse) {
        GPIOA->ODR |= GPIO_ODR_3;
      }
    }
    
    if(!(GPIOA->IDR & GPIO_IDR_1)) {
      if(tmr_in1_delay < 10000) {
        tmr_in1_delay++;
      }
    }
    
    //reset relay
    if((GPIOA->IDR & GPIO_IDR_4) && (GPIOA->IDR & GPIO_IDR_5)) {
      GPIOF->ODR &= ~GPIO_ODR_0;
      state_relay_0 = RELAY_OFF;
    }
    if((GPIOA->IDR & GPIO_IDR_6) && (GPIOA->IDR & GPIO_IDR_7)) {
      GPIOF->ODR &= ~GPIO_ODR_1;
      state_relay_1 = RELAY_OFF;
    }
  }
}

int main() {
  volatile char build_str[] = {
    'V', 'e','r',':',
    BUILD_MAJOR + '0', '.' , BUILD_MINOR + '0', '.',
    __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],
    '\0'
  };

  int _wait = 0xFFFF;
  
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SYSCFGEN;
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOFEN;
    
  SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI1_PA | SYSCFG_EXTICR1_EXTI0_PA;  
  SYSCFG->EXTICR[1] = SYSCFG_EXTICR2_EXTI7_PA | SYSCFG_EXTICR2_EXTI6_PA | SYSCFG_EXTICR2_EXTI5_PA | SYSCFG_EXTICR2_EXTI4_PA;
  
  EXTI->IMR |= EXTI_IMR_MR7 | EXTI_IMR_MR6 | EXTI_IMR_MR5 | EXTI_IMR_MR4 | EXTI_IMR_MR1 | EXTI_IMR_MR0;
  EXTI->RTSR |= EXTI_RTSR_TR7 | EXTI_RTSR_TR6 | EXTI_RTSR_TR5 | EXTI_RTSR_TR4 | EXTI_RTSR_TR1 | EXTI_RTSR_TR0;
  EXTI->FTSR |= EXTI_FTSR_TR7 | EXTI_FTSR_TR6 | EXTI_FTSR_TR5 | EXTI_FTSR_TR4 | EXTI_FTSR_TR1 | EXTI_FTSR_TR0;
  
  GPIOA->MODER |= GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0;
  GPIOF->MODER = 0;
  GPIOF->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0;
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR3;
  GPIOF->OSPEEDR = 0;
  GPIOF->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0 | GPIO_OSPEEDER_OSPEEDR1;
  
  GPIOF->ODR &= ~GPIO_ODR_0;
  GPIOF->ODR &= ~GPIO_ODR_1;
  
  //timer
  TIM1->CR1 |= TIM_CR1_CEN;
  TIM1->DIER |= TIM_DIER_UIE;
  TIM1->CNT = 57535;
   
  //while(_wait--);
   
  EXTI->PR = 0;  
  GPIOA->ODR |= GPIO_ODR_2;
  GPIOA->ODR |= GPIO_ODR_3;
  
  NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_EnableIRQ(EXTI4_15_IRQn);
  
  NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn,0); 
  NVIC_SetPriority(EXTI0_1_IRQn,0); 
  NVIC_SetPriority(EXTI4_15_IRQn,0); 
    
  while(1);
	
  return 0;
}