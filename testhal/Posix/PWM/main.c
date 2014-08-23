#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ch.h"
#include "hal.h"

void show_pwm(void) {
	printf("pwm state:\t%d\r\n", PWMD1.state);
	printf("pwm freq:\t%d\r\n", PWMD1.config->frequency);
	puts("");
}

#define INIT_PWM_FREQ 1000000 /* 1 Mhz PWM clock frequency, 1 uS per tick */
#define INIT_PWM_PERIOD 400   /* PWM period in freq ticks, 400uS */
#define INIT_PWM_WIDTH_TICS 20

PWMConfig pwmcfg_led = {
	INIT_PWM_FREQ,    /* Frequency */
	INIT_PWM_PERIOD, /* Period */
	NULL,            /* Callback (Not used here) */
	{
		{PWM_OUTPUT_DISABLED, NULL},
		{PWM_OUTPUT_DISABLED, NULL},
		{PWM_OUTPUT_DISABLED, NULL},
		{PWM_OUTPUT_ACTIVE_HIGH, NULL}, /* Only channel 4 enabled */
	},
};

int main(void) {
	halInit();
	chSysInit();
	pwmStart(&PWMD1, &pwmcfg_led);

	unsigned int pwm_period_new = INIT_PWM_PERIOD;
	bool upcount = TRUE;

	show_pwm();

	while (TRUE) {
		pwmEnableChannel(&PWMD1, 3, INIT_PWM_WIDTH_TICS);


		if(upcount) {
			pwm_period_new += 1;
		} else {
			pwm_period_new -= 1;
		}

		if(pwm_period_new > (INIT_PWM_PERIOD * 0.5)) {
			upcount = FALSE;
		}

		if(pwm_period_new < (INIT_PWM_WIDTH_TICS+1)) {
			upcount = TRUE;
		}

		pwmChangePeriod(&PWMD1, pwm_period_new);

		pwmDisableChannel(&PWMD1, 3);

		chThdSleep(MS2ST(20));
	}

	return 0;
}
