#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

/* Struct para los datos de temperatura */
typedef struct {
  float lm35;
  float pote;
} temperature_data_t;

/* Queue para comunicar los dos nucleos */
queue_t queue;

/* Main para el core 1 */
void core1_main() {
  
    while(1) {
        /* Variable para recuperar el dato de la queue */
        temperature_data_t data;
        /* Espera a que esten los datos para recibir */
        queue_remove_blocking(&queue, &data);
            int DT = (data.pote - data.lm35);
             if (DT > 10){
             DT = 10;
            }
            else if (DT < -10) {
            DT = 10;
            }
            else if (DT < 0) {
            DT = DT * -1;
            }

            DT = DT * 0xffff / 10;
            if (data.pote > data.lm35){
            pwm_set_gpio_level(9, DT);
            pwm_set_gpio_level(8, 0);
            }
            if (data.lm35 > data.pote){
            pwm_set_gpio_level(8, DT);
            pwm_set_gpio_level(9, 0);
            }
            printf("TempDif %d\n", DT);
        }

    }
}

/* Main para el core 0 */
int main() {
    stdio_init_all();
    /* Inicializa la cola para enviar un unico dato */
    queue_init(&queue, sizeof(temperature_data_t), 1);
    /* Inicializa el core 1 */
    multicore_launch_core1(core1_main);
    float v1, v2;
    adc_init();
    adc_gpio_init(28);
    adc_gpio_init(27);
    
    gpio_set_function(9, GPIO_FUNC_PWM);
    gpio_set_function(8, GPIO_FUNC_PWM);

    uint8_t slice_num = pwm_gpio_to_slice_num(9);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    
    while(1) {
        /* Variable para enviar los datos */
        temperature_data_t data;
        adc_select_input(1);
        uint16_t poten = adc_read();
        adc_select_input(2);
        uint16_t lm = adc_read();
        v1= (float) poten * 3.3/4095;
        data.pote = v1 * 35/3.3; 
        v2= (float) lm * 3.3 / 4095;
        data.lm35 = v2*100;
        
      
        /* Cuando los datos estan listos, enviar por la queue */
        queue_add_blocking(&queue, &data);
    }
}
