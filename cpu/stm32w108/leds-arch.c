/*
 * Copyright (c) 2010, STMicroelectronics.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 * $Id: leds-arch.c,v 1.1 2010/10/25 09:03:38 salvopitru Exp $
 */
/*---------------------------------------------------------------------------*/
/**
* \file
*			Leds.
* \author
*			Salvatore Pitrulli <salvopitru@users.sourceforge.net>
*/
/*---------------------------------------------------------------------------*/

#include "contiki-conf.h"
#include "dev/leds.h"


/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{   
  LED_CONFIG();
  
  LEDS_PORT |= (LEDS_CONF_RED | LEDS_CONF_GREEN);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
    return ((LEDS_PORT & LEDS_CONF_RED) ? 0 : LEDS_RED)
    | ((LEDS_PORT & LEDS_CONF_GREEN) ? 0 : LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
    
  LEDS_PORT = (LEDS_PORT & ~(LEDS_CONF_RED|LEDS_CONF_GREEN))
    | ((leds & LEDS_RED) ? 0 : LEDS_CONF_RED)
    | ((leds & LEDS_GREEN) ? 0 : LEDS_CONF_GREEN);
 
}
/*---------------------------------------------------------------------------*/