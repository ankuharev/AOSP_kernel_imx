 /*
  * rt5610.c  --  ALSA Soc RT5610 codec support
  *
  *  Copyright 2011 Realtek Semiconductor Corp.
  *  Author: Johnny Hsu <johnnyhsu@realtek.com>
  *
  *  This program is free software; you can redistribute  it and/or modify it
  *  under  the terms of  the GNU General  Public License as published by the
  *  Free Software Foundation;  either version 2 of the  License, or (at your
  *  option) any later version.
  */
 
 #include <linux/init.h>
 #include <linux/slab.h>
 #include <linux/module.h>
 #include <linux/device.h>
 #include <sound/core.h>
 #include <sound/pcm.h>
 #include <sound/ac97_codec.h>
 #include <sound/initval.h>
 #include <sound/pcm_params.h>
 #include <sound/tlv.h>
 #include <sound/soc.h>
 
 #include "rt5610.h"
 #define AUDIO_NAME "rt5610"
 
 #ifdef RT5610_DEBUG
 #define dbg(format, arg...) \
 	printk(KERN_DEBUG AUDIO_NAME ": " format "\n" , ## arg)
 #else
 #define dbg(format, arg...) do {} while (0)
 #endif
 #define err(format, arg...) \
 	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
 #define info(format, arg...) \
 	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
 #define warn(format, arg...) \
 	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)
 
 
 /* codec private data */
 struct rt5610_priv {
 	unsigned int pll_id;
 	unsigned int pll_in;
 	unsigned int pll_out;
 	unsigned int vpcm_sysclk;
 };
 
 static int rt5610_set_pll(struct snd_soc_codec *codec, 
 		int pll_id, unsigned int freq_in, unsigned int freq_out);
 
 u16 Set_Codec_Reg_Init[][2]={
 
 	{RT5610_SPK_OUT_VOL		,0x8080},//default speaker volume to 0db 
 	{RT5610_HP_OUT_VOL		,0x8888},//default HP volume to 0db
 	{RT5610_ADC_REC_MIXER	,0x3F3F},//default Record is Mic1
 	{RT5610_STEREO_DAC_VOL	,0x0808},//default stereo DAC volume to 0db
 	{RT5610_MIC_CTRL		,0x0500},//set boost to +20DB
 	{RT5610_INDEX_ADDRESS 	,0x0054},//AD_DA_Mixer_internal Register5
 	{RT5610_INDEX_DATA		,0xE184},//To reduce power consumption for DAC reference
 	{RT5610_TONE_CTRL		,0x0001},//Enable varible sample rate	
 	{RT5610_OUTPUT_MIXER_CTRL,0x4b40},//default output mixer control,CLASS AB
 	{RT5610_ADC_REC_GAIN	,0xFB16},//gain 16.5db to ADC gain
 };
 
 /*
  * rt5610 register cache
  * We can't read the RT5610 register space when we
  * are using 2 wire for device control, so we cache them instead.
  */
 static const u16 rt5610_reg[] = {
 	0x59b4, 0x8080, 0x8080, 0x0000, // 6
 	0xc880, 0xe808, 0xe808, 0x0808, // e
 	0xe0e0, 0xf58b, 0x7f7f, 0x0000, // 16
 	0xe800, 0x0000, 0x0000, 0x0000, // 1e
 	0x0000, 0x0000, 0x0000, 0xef00, // 26
 	0x0000, 0x0000, 0xbb80, 0x0000, // 2e
 	0x0000, 0xbb80, 0x0000, 0x0000, // 36
 	0x0000, 0x0000, 0x0000, 0x0000, // 3e
 	0x0428, 0x0000, 0x0000, 0x0000, // 46
 	0x0000, 0x0000, 0x2e3e, 0x2e3e, // 4e
 	0x0000, 0x0000, 0x003a, 0x0000, // 56
 	0x0cff, 0x0000, 0x0000, 0x0000, // 5e
 	0x0000, 0x0000, 0x2130, 0x0010, // 66
 	0x0053, 0x0000, 0x0000, 0x0000, // 6e
 	0x0000, 0x0000, 0x008c, 0x3f00, // 76
 	0x0000, 0x0000, 0x10ec, 0x1003, // 7e
 };
 
 
 
 /* virtual HP mixers regs */
 #define HPL_MIXER	0x80
 #define HPR_MIXER	0x82
 
 static u16 reg80=0x2,reg82=0x2;
 
 /*
  * read rt5610 register cache
  */
 static inline unsigned int rt5610_read_reg_cache(struct snd_soc_codec *codec,
 		unsigned int reg)
 {
 	u16 *cache = codec->reg_cache;
 	if (reg < 1 || reg > (ARRAY_SIZE(rt5610_reg) + 1))
 		return -1;
 	return cache[reg/2];
 }
 
 
 /*
  * write rt5610 register cache
  */
 
 static inline void rt5610_write_reg_cache(struct snd_soc_codec *codec,
 		unsigned int reg, unsigned int value)
 {
 	u16 *cache = codec->reg_cache;
 	if (reg < 0 || reg > 0x7e)
 		return;
 	cache[reg/2] = value;
 }
 
 
 
 static unsigned int rt5610_read(struct snd_soc_codec *codec, 
 		unsigned int reg)
 {
 //	printk(KERN_INFO "%s reg=0x%x, \n", __func__, reg);
 	if (reg == 0x80)
 		return reg80;
 	else if (reg == 0x82)
 		return reg82;
 
 	return soc_ac97_ops.read(codec->ac97, reg);	
 }
 
 static int rt5610_write(struct snd_soc_codec *codec, unsigned int reg,
 		unsigned int val)
 {
 	u16 *cache = codec->reg_cache;
 
 //	printk(KERN_INFO "%s reg=0x%x, val=0x%x\n", __func__, reg, val);
 	if (reg == 0x80) {
 		reg80 = val;
 		return 0;
 	}
 	else if (reg == 0x82){
 		reg82 = val;
 		return 0;
 	}
 
 	if (reg < 0x7c)
 		soc_ac97_ops.write(codec->ac97, reg, val);
 	reg = reg >> 1;
 	if (reg < (ARRAY_SIZE(rt5610_reg)))
 		cache[reg] = val;
 	//#endif
 	return 0;
 }
 
 int rt5610_reset(struct snd_soc_codec *codec, int try_warm)
 {
 	if (try_warm && soc_ac97_ops.warm_reset) {
 		soc_ac97_ops.warm_reset(codec->ac97);
 		if (rt5610_read(codec, 0) == rt5610_reg[0])
 			return 1;
 	}
 
 	if (soc_ac97_ops.reset)
 		soc_ac97_ops.reset(codec->ac97);
 
 	if(rt5610_read(codec, 0) != rt5610_reg[0]){
 		return -EIO;
 	}
 	return 0;
 }
 EXPORT_SYMBOL_GPL(rt5610_reset);
 
 #define rt5610_write_mask(c, reg, value, mask) snd_soc_update_bits(c, reg, mask, value)
 
 static int rt5610_init(struct snd_soc_codec *codec)
 {
 	int i;
 
 	//power on PR bits
 	rt5610_write(codec,RT5610_PD_CTRL_STAT,0);	
 	//power on main bias of codec
 	rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_MAIN_BIAS|PWR_MIC_BIAS1,PWR_MAIN_BIAS|PWR_MIC_BIAS1);	
 	//power on vref of codec
 	rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_MIXER_VREF,PWR_MIXER_VREF);	
 
 	//	rt5610_set_pll(codec,0,13000000,24576000);//input 13Mhz,output 24.576Mhz
 
 	for(i=0;i<ARRAY_SIZE(Set_Codec_Reg_Init);i++)
 	{
 		rt5610_write(codec,Set_Codec_Reg_Init[i][0],Set_Codec_Reg_Init[i][1]);
 
 	}
 
 	return 0;
 }
 
 static const char *rt5610_spkl_pga[] = {"Vmid","HPL mixer","SPK mixer","Mono Mixer"};
 static const char *rt5610_spkr_pga[] = {"Vmid","HPR mixer","SPK mixer","Mono Mixer"};
 static const char *rt5610_hpl_pga[]  = {"Vmid","HPL mixer"};
 static const char *rt5610_hpr_pga[]  = {"Vmid","HPR mixer"};
 static const char *rt5610_mono_pga[] = {"Vmid","HP mixer","SPK mixer","Mono Mixer"};
 static const char *rt5610_amp_type_sel[] = {"Class AB","Class D"};
 static const char *rt5610_mic_boost_sel[] = {"Bypass","+20db","+30db","+40db"};
 
 static const struct soc_enum rt5610_enum[] = {
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 14, 4, rt5610_spkl_pga), /* spk left input sel 0 */	
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 11, 4, rt5610_spkr_pga), /* spk right input sel 1 */	
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 9, 2, rt5610_hpl_pga), /* hp left input sel 2 */	
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 8, 2, rt5610_hpr_pga), /* hp right input sel 3 */	
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 6, 4, rt5610_mono_pga), /* mono input sel 4 */
 	SOC_ENUM_SINGLE(RT5610_MIC_CTRL, 10, 4, rt5610_mic_boost_sel), /*Mic1 boost sel 5 */
 	SOC_ENUM_SINGLE(RT5610_MIC_CTRL, 8, 4, rt5610_mic_boost_sel), /*Mic2 boost sel 6 */
 	SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 13, 2, rt5610_amp_type_sel), /*Speaker AMP sel 7 */
 };
 
 static int rt5610_amp_sel_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
 {
 	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
 	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
 	unsigned short val;
 	unsigned short mask, bitmask;
 
 	for (bitmask = 1; bitmask < e->max; bitmask <<= 1)
 		;
 	if (ucontrol->value.enumerated.item[0] > e->max - 1)
 		return -EINVAL;
 	val = ucontrol->value.enumerated.item[0] << e->shift_l;
 	mask = (bitmask - 1) << e->shift_l;
 	snd_soc_update_bits(codec, e->reg, mask, val);
 	if (ucontrol->value.enumerated.item[0] == 0)// Class AB
 		rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD2, PWR_CLASS_AB, PWR_CLASS_AB);
 	else// Class AB
 		rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD2, 0, PWR_CLASS_AB);
 
 	return 0;
 }
 
 
 static const struct snd_kcontrol_new rt5610_snd_controls[] = {
 	SOC_DOUBLE("Speaker Playback Volume", RT5610_SPK_OUT_VOL, 8, 0, 31, 0),
 	SOC_DOUBLE("Speaker Playback Switch", RT5610_SPK_OUT_VOL, 15, 7, 1, 1),
 	SOC_DOUBLE("Headphone Playback Volume", RT5610_HP_OUT_VOL, 8, 0, 31, 0),
 	SOC_DOUBLE("Headphone Playback Switch", RT5610_HP_OUT_VOL,15, 7, 1, 1),
 	SOC_SINGLE("Mono Playback Volume", RT5610_PHONEIN_MONO_OUT_VOL, 0, 31, 0),
 	SOC_SINGLE("Mono Playback Switch", RT5610_PHONEIN_MONO_OUT_VOL, 7, 1, 1),
 	SOC_DOUBLE("PCM Playback Volume", RT5610_STEREO_DAC_VOL, 0, 0, 31, 0),
 	SOC_SINGLE("PCM Playback Switch", RT5610_STEREO_DAC_VOL,15, 1, 1),
 	SOC_DOUBLE("Line In Volume", RT5610_LINE_IN_VOL, 8, 0, 31, 0),
 	SOC_SINGLE("Line In Switch", RT5610_LINE_IN_VOL,15, 1, 1),
 	SOC_SINGLE("Mic 1 Volume", RT5610_MIC_VOL, 8, 31, 0),
 	SOC_SINGLE("Mic 2 Volume", RT5610_MIC_VOL, 0, 31, 0),
 	SOC_ENUM("Mic 1 Boost", rt5610_enum[5]),
 	SOC_ENUM("Mic 2 Boost", rt5610_enum[6]),
 	SOC_ENUM_EXT("Speaker Amp Type", rt5610_enum[7], snd_soc_get_enum_double, rt5610_amp_sel_put),
 	SOC_SINGLE("Phone In Volume", RT5610_PHONEIN_MONO_OUT_VOL, 8, 31, 0),
 	SOC_DOUBLE("Capture Volume", RT5610_ADC_REC_GAIN, 0, 0, 31, 0),
 };
 
 static int rt5610_ChangeCodecPowerStatus(struct snd_soc_codec *codec,int power_state)
 {
 	unsigned short int PowerDownState=0;
 
 	switch(power_state)
 	{
 		case POWER_STATE_D0:			//FULL ON-----power on all power
 
 			rt5610_write(codec,RT5610_PD_CTRL_STAT,PowerDownState);
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD1,~PowerDownState);
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD2,~PowerDownState);
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD3,~PowerDownState);
 
 			break;	
 
 		case POWER_STATE_D1:		//LOW ON-----
 
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_DAC_REF|PWR_MIC_BIAS1|PWR_HI_R_LOAD_HP
 					,PWR_DAC_REF|PWR_MIC_BIAS1|PWR_HI_R_LOAD_HP);
 
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
 					PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB
 					,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
 					PWR_R_DAC_CLK | PWR_L_DAC_CLK | PWR_CLASS_AB);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_MIC1_BOOST  | PWR_SPK_R_OUT| PWR_SPK_L_OUT |PWR_HP_R_OUT | PWR_HP_L_OUT |
 					PWR_SPK_RN_OUT|PWR_SPK_LN_OUT 
 					,PWR_MIC1_BOOST | PWR_SPK_R_OUT| PWR_SPK_L_OUT |PWR_HP_R_OUT | PWR_HP_L_OUT	|
 					PWR_SPK_RN_OUT|PWR_SPK_LN_OUT);								
 			break;
 
 		case POWER_STATE_D1_PLAYBACK:	//Low on of Playback
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_DAC_REF | PWR_HI_R_LOAD_HP |PWR_HI_R_LOAD_MONO,PWR_DAC_REF | PWR_HI_R_LOAD_HP|PWR_HI_R_LOAD_MONO);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB
 					,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_SPK_L_OUT|PWR_SPK_R_OUT | PWR_HP_R_OUT | PWR_HP_L_OUT|PWR_SPK_RN_OUT|PWR_SPK_LN_OUT|PWR_MONO_VOL
 					,PWR_SPK_L_OUT|PWR_SPK_R_OUT | PWR_HP_R_OUT | PWR_HP_L_OUT|PWR_SPK_RN_OUT|PWR_SPK_LN_OUT|PWR_MONO_VOL);	
 
 			break;
 
 		case POWER_STATE_D1_RECORD:	//Low on of Record
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_MIC_BIAS1|PWR_MIC_BIAS2,PWR_MIC_BIAS1|PWR_MIC_BIAS2);
 			//
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN												
 					,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN);
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_MIC1_BOOST|PWR_MIC2_BOOST ,PWR_MIC1_BOOST|PWR_MIC2_BOOST);	
 
 
 
 			break;
 
 		case POWER_STATE_D2:		//STANDBY----
 			//																								
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_DAC_REF | PWR_HI_R_LOAD_HP |PWR_HI_R_LOAD_MONO);
 			//
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
 					PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_MIC1_BOOST | PWR_MIC1_VOL_CTRL | PWR_SPK_R_OUT | PWR_SPK_L_OUT/* |PWR_HP_R_OUT | PWR_HP_L_OUT*/ |
 					PWR_SPK_RN_OUT | PWR_SPK_LN_OUT);	
 
 			break;
 
 		case POWER_STATE_D2_PLAYBACK:	//STANDBY of playback
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_DAC_REF | PWR_HI_R_LOAD_HP |PWR_HI_R_LOAD_MONO);
 			//
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_SPK_R_OUT |PWR_SPK_L_OUT /*| PWR_HP_R_OUT | PWR_HP_L_OUT*/ | PWR_SPK_RN_OUT| PWR_SPK_LN_OUT|PWR_MONO_VOL);
 
 			break;
 
 		case POWER_STATE_D2_RECORD:		//STANDBY of record
 
 			//			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_MIC_BIAS1);
 			//
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN);
 
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_MIC1_BOOST | PWR_MIC1_VOL_CTRL);	
 
 			break;		
 
 		case POWER_STATE_D3:		//SLEEP
 		case POWER_STATE_D4:		//OFF----power off all power,include PR0,PR1,PR3,PR4,PR5,PR6,EAPD,and addition power managment
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD3,PowerDownState);
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD1,PowerDownState);
 			rt5610_write(codec,RT5610_PWR_MANAG_ADD2,PowerDownState);
 
 
 			PowerDownState=RT_PWR_PR0 | RT_PWR_PR1 | RT_PWR_PR2 | RT_PWR_PR3 /*| RT_PWR_PR4*/ | RT_PWR_PR5 | RT_PWR_PR6 | RT_PWR_PR7; 		
 			rt5610_write(codec,RT5610_PD_CTRL_STAT,PowerDownState);		
 
 			break;	
 
 		default:
 
 			break;
 	}
 
 	return 0;	
 }
 
 static int rt5610_AudioOutEnable(struct snd_soc_codec *codec,unsigned short int WavOutPath,int Mute)
 {	
 
 	if(Mute)
 	{
 		switch(WavOutPath)
 		{
 			case RT_WAVOUT_ALL_ON:
 
 				rt5610_write_mask(codec,RT5610_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
 				rt5610_write_mask(codec,RT5610_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
 				rt5610_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,RT_R_MUTE,RT_R_MUTE);				//Mute Mono channel
 				rt5610_write_mask(codec,RT5610_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
 						,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
 
 				break;
 
 			case RT_WAVOUT_HP:
 
 				rt5610_write_mask(codec,RT5610_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
 
 				break;
 
 			case RT_WAVOUT_SPK:
 
 				rt5610_write_mask(codec,RT5610_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel			
 
 				break;
 
 
 			case RT_WAVOUT_MONO:
 
 				rt5610_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,RT_R_MUTE,RT_R_MUTE);	//Mute MonoOut channel		
 
 				break;
 
 			case RT_WAVOUT_DAC:
 
 				rt5610_write_mask(codec,RT5610_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
 						,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
 
 				break;
 			default:
 
 				return 0;
 
 		}
 	}
 	else
 	{
 		switch(WavOutPath)
 		{
 
 			case RT_WAVOUT_ALL_ON:
 
 				rt5610_write_mask(codec,RT5610_SPK_OUT_VOL	,0,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
 				rt5610_write_mask(codec,RT5610_HP_OUT_VOL 		,0,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
 				rt5610_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//Mute Mono channel
 				rt5610_write_mask(codec,RT5610_STEREO_DAC_VOL	,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
 
 				break;
 
 			case RT_WAVOUT_HP:
 
 				rt5610_write_mask(codec,RT5610_HP_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//unMute headphone right/left channel
 
 				break;
 
 			case RT_WAVOUT_SPK:
 
 				rt5610_write_mask(codec,RT5610_SPK_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//unMute Speaker right/left channel			
 
 				break;			
 
 			case RT_WAVOUT_MONO:
 
 				rt5610_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,0,RT_R_MUTE);	//unMute MonoOut channel		
 
 				break;
 			case RT_WAVOUT_DAC:
 
 				rt5610_write_mask(codec,RT5610_STEREO_DAC_VOL,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//unMute DAC to HP,Speaker,Mono Mixer
 
 				break;
 			default:
 				return 0;
 		}
 
 	}
 
 	return 0;
 }
 
 static int rt5610_pcm_hw_prepare(struct snd_pcm_substream *substream,
 		struct snd_soc_dai *codec_dai)
 {
 	struct snd_soc_codec *codec = codec_dai->codec;
 	int stream = substream->stream;
 
 	switch (stream) {
 		case SNDRV_PCM_STREAM_PLAYBACK:
 			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D1_PLAYBACK);//power on dac,mixer and output power
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_SPK,0);	//unmute speaker
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_HP,0);	//unmute hp out
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_MONO,0);	//unmute monoout
 			break;
 
 		case SNDRV_PCM_STREAM_CAPTURE:
 			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D1_RECORD);	//power on adc ,mixer and input power
 			rt5610_write_mask(codec, RT5610_ADC_REC_MIXER,0x0000,0x4040);//enable reocrd source from mic1
 			break;
 	}
 	return 0;
 }
 
 
 /* PLL divisors */
 struct _pll_div {
 	u32 pll_in;
 	u32 pll_out;
 	u16 regvalue;
 };
 
 static const struct _pll_div codec_pll_div[] = {
 
 	{  2048000,  8192000,	0x0ea0},
 	{  3686400,  8192000,	0x4e27},
 	{ 12000000,  8192000,	0x456b},
 	{ 13000000,  8192000,	0x495f},
 	{ 13100000,  8192000,	0x0320},
 	{  2048000,  11289600,	0xf637},
 	{  3686400,  11289600,	0x2f22},
 	{ 12000000,  11289600,	0x3e2f},
 	{ 13000000,  11289600,	0x4d5b},
 	{ 13100000,  11289600,	0x363b},
 	{  2048000,  16384000,	0x1ea0},
 	{  3686400,  16384000,	0x9e27},
 	{ 12000000,  16384000,	0x452b},
 	{ 13000000,  16384000,	0x542f},
 	{ 13100000,   16384000,	0x03a0},
 	{  2048000,  16934400,	0xe625},
 	{  3686400,  16934400,	0x9126},
 	{ 12000000,  16934400,	0x4d2c},
 	{ 13000000,  16934400,	0x742f},
 	{ 13100000,  16934400,	0x3c27},
 	{  2048000,  22579200,	0x2aa0},
 	{  3686400,  22579200,	0x2f20},
 	{ 12000000,  22579200,	0x7e2f},
 	{ 13000000,  22579200,	0x742f},
 	{ 13100000,  22579200,	0x3c27},	
 	{  2048000,  24576000,	0x2ea0},
 	{  3686400,  24576000,	0xee27},
 	{ 12000000,  24576000,	0x2915},
 	{ 13000000,  24576000,	0x772e},
 	{ 13100000,  24576000,	0x0d20},
 };
 
 
 static int rt5610_set_pll(struct snd_soc_codec *codec, 
 		int pll_id, unsigned int freq_in, unsigned int freq_out)
 {
 	int ret = -EINVAL;
 	int i;
 
 	if (pll_id != RT5610_PLL_FR_MCLK)
 		return -EINVAL;
 
 	if (!freq_in || !freq_out)
 		return 0;
 
 	for (i = 0; i < ARRAY_SIZE(codec_pll_div); i++) {
 		if (codec_pll_div[i].pll_in == freq_in && codec_pll_div[i].pll_out == freq_out)
 		{
 			rt5610_write_mask(codec,RT5610_GEN_CTRL_REG1, GP_CLK_FROM_PLL, GP_CLK_FROM_PLL);//Codec sys-clock from PLL 
 
 			rt5610_write(codec,RT5610_PLL_CTRL, codec_pll_div[i].regvalue);//set PLL parameter 	
 			//enable PLL power	
 			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2, PWR_PLL, PWR_PLL);						
 			//Power off ACLink	
 			rt5610_write_mask(codec,RT5610_PD_CTRL_STAT, RT_PWR_PR4, RT_PWR_PR4);	 		
 			//need ac97 controller to do warm reset	
 			soc_ac97_ops.warm_reset(codec->ac97);
 
 			/* wait 10ms AC97 link frames for the link to stabilise */
 			schedule_timeout_interruptible(msecs_to_jiffies(10));	
 
 			ret = 0;
 		}
 	}
 	return ret;
 }
 
 static int rt5610_set_dai_pll(struct snd_soc_dai *codec_dai,
 		int pll_id, int source, unsigned int freq_in, unsigned int freq_out)
 {
 	int ret;
 	struct snd_soc_codec *codec = codec_dai->codec;
 	struct rt5610_priv *rt5610 = snd_soc_codec_get_drvdata(codec);
 
 	ret =rt5610_set_pll(codec, pll_id, freq_in, freq_out);
 
 	if (ret < 0) {
 		printk(KERN_ERR "pll unmatched\n");
 		return 0;
 	}
 
 	rt5610->pll_id = pll_id;
 	rt5610->pll_in = freq_in;
 	rt5610->pll_out = freq_out;
 
 	return ret;
 }
 
 
 static int rt5610_pcm_hw_params(struct snd_pcm_substream *substream,
 		struct snd_pcm_hw_params *params, struct snd_soc_dai *codec_dai)
 {
 	struct snd_soc_codec *codec = codec_dai->codec;
 	int stream = substream->stream;
 	int rate = params_rate(params);
 	int reg;
 	u16 vra;
 
 	vra = rt5610_read(codec, RT5610_TONE_CTRL);
 	rt5610_write(codec, RT5610_TONE_CTRL, vra | 0x1);
 
 	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
 		reg = RT5610_STEREO_DAC_RATE;
 	else
 		reg = RT5610_STEREO_ADC_RATE;
 
 	rt5610_write(codec, reg,rate);
 
 	return 0;
 }
 
 
 
 static void rt5610_pcm_shutdown(struct snd_pcm_substream *substream,
 		struct snd_soc_dai *codec_dai)
 {
 	struct snd_soc_codec *codec = codec_dai->codec;
 	int stream = substream->stream;
 
 	switch (stream) {
 		case SNDRV_PCM_STREAM_PLAYBACK:
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_SPK,1);	//mute speaker out
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_HP,1);	//mute hp out
 			rt5610_AudioOutEnable(codec,RT_WAVOUT_MONO,1);	//mute hp out
 			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D2_PLAYBACK);	//power off dac to hp and speaker out
 			break;
 
 		case SNDRV_PCM_STREAM_CAPTURE:
 			rt5610_write_mask(codec, RT5610_ADC_REC_MIXER,0x4040,0x4040);	//disable reocrd source from mic1
 			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D2_RECORD);		//power off adc and mixer and input power
 			break;
 	}
 }
 
 
 static int rt5610_set_bias_level(struct snd_soc_codec *codec, 
 		enum snd_soc_bias_level level)
 {
 	switch (level) {
 		case SND_SOC_BIAS_ON:
 			break;
 		case SND_SOC_BIAS_PREPARE:
 			rt5610_write(codec, RT5610_PD_CTRL_STAT, 0);
 			rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD1, PWR_MAIN_BIAS, PWR_MAIN_BIAS);//main bias
 			rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD2, PWR_MIXER_VREF, PWR_MIXER_VREF);//vref
 			break;
 		case SND_SOC_BIAS_STANDBY:
 			break;		
 		case SND_SOC_BIAS_OFF:
 			rt5610_write_mask(codec, RT5610_SPK_OUT_VOL, RT_L_MUTE | RT_R_MUTE, RT_L_MUTE | RT_R_MUTE);
 			rt5610_write_mask(codec, RT5610_HP_OUT_VOL, RT_L_MUTE | RT_R_MUTE, RT_L_MUTE | RT_R_MUTE);
 			rt5610_write_mask(codec, RT5610_PHONEIN_MONO_OUT_VOL, RT_R_MUTE, RT_R_MUTE);
 			rt5610_write(codec, RT5610_PWR_MANAG_ADD1, 0x0000);
 			rt5610_write(codec, RT5610_PWR_MANAG_ADD2, 0x0000);
 			rt5610_write(codec, RT5610_PWR_MANAG_ADD3, 0x0000);
 			break;		
 	}
 	codec->dapm.bias_level = level;
 
 	return 0;
 }
 
 #define RT5610_HIFI_RATES SNDRV_PCM_RATE_8000_48000
 #define RT5610_HIFI_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
 #define RT5610_VOICE_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
 		SNDRV_PCM_FMTBIT_S24_LE)
 
 static struct snd_soc_dai_ops rt5610_hifi_ops = {
 	.hw_params = rt5610_pcm_hw_params,
 	.set_pll = rt5610_set_dai_pll,
 	.shutdown = rt5610_pcm_shutdown,
 	.prepare = rt5610_pcm_hw_prepare,
 };
 
 static struct snd_soc_dai_driver rt5610_dai[] = {
 	{
 		.name = "rt5610-hifi",
 		.ac97_control = 1,
 		.playback = {
 			.stream_name = "HiFi Playback",
 			.channels_min = 1,
 			.channels_max = 2,
 			.rates = RT5610_HIFI_RATES,
 			.formats = RT5610_HIFI_FORMATS,
 		},
 		.capture = {
 			.stream_name = "HiFi Capture",
 			.channels_min = 1,
 			.channels_max = 2,
 			.rates = RT5610_HIFI_RATES,
 			.formats = RT5610_HIFI_FORMATS,
 		},
 		.ops = &rt5610_hifi_ops,
 	},
 	{
 		.name = "RT5610 Reserved",
 		.id = 2,
 	},
 };
 
 static int rt5610_suspend(struct snd_soc_codec *codec,
 		pm_message_t state)
 {
 	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
 	return 0;
 }
 
 static int rt5610_resume(struct snd_soc_codec *codec)
 {
 
 
 	struct rt5610_priv *rt5610 = snd_soc_codec_get_drvdata(codec);
 	u16 *cache = codec->reg_cache;
 	int i;
 
 	/* do we need to re-start the PLL ? */
 	if (rt5610->pll_out)
 		rt5610_set_pll(codec, rt5610->pll_id, rt5610->pll_in, rt5610->pll_out);
 
 	for (i = 0; i < ARRAY_SIZE(rt5610_reg); i++) {
 		if (i == RT5610_RESET)
 			continue;
 		soc_ac97_ops.write(codec->ac97, i << 1, cache[i]);
 	}
 
 	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
 
 	return 0;
 }
 
 
 /*
  * initialise the RT5610 driver
  * register the mixer and dsp interfaces with the kernel
  */
 static int rt5610_probe(struct snd_soc_codec *codec)
 {
 	struct rt5610_priv *rt5610;
 	int ret = 0;
 
 	rt5610 = kzalloc(sizeof(struct rt5610_priv), GFP_KERNEL);
 	if (rt5610 == NULL)
 		return -ENOMEM;
 	snd_soc_codec_set_drvdata(codec, rt5610);
 
 	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0); 
 	if (ret < 0)
 		goto codec_err;
 
 	/* do a cold reset for the controller and then try
 	 * a warm reset followed by an optional cold reset for codec */
 	rt5610_reset(codec, 0);
 	ret = rt5610_reset(codec, 1);
 	if (ret < 0) {
 		dev_err(codec->dev,
 				"FAIL to reset rt5610\n");
 		goto reset_err;
 	}
 	rt5610_write(codec, RT5610_PD_CTRL_STAT, 0);
 	rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD1, PWR_MAIN_BIAS, PWR_MAIN_BIAS);//main bias
 	rt5610_write_mask(codec, RT5610_PWR_MANAG_ADD2, PWR_MIXER_VREF, PWR_MIXER_VREF);//vref
 	rt5610_init(codec);
 
 	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
 
 	snd_soc_add_codec_controls(codec, rt5610_snd_controls,
 			ARRAY_SIZE(rt5610_snd_controls));
 
 	return 0;
 
 reset_err:
 	snd_soc_free_ac97_codec(codec);
 codec_err:
 	kfree(rt5610);
 	return ret;
 }
 
 static int rt5610_remove(struct snd_soc_codec *codec)
 {
 	struct rt5610_priv *rt5610 = snd_soc_codec_get_drvdata(codec);
 	snd_soc_free_ac97_codec(codec);
 	kfree(rt5610);
 	return 0;
 }
 
 static struct snd_soc_codec_driver soc_codec_dev_rt5610 = {
 	.probe = rt5610_probe,
 	.remove = rt5610_remove,
 	.suspend = rt5610_suspend,
 	.resume = rt5610_resume,
 	.read = rt5610_read,
 	.write = rt5610_write,
 	.set_bias_level = rt5610_set_bias_level,
 	.reg_cache_size = ARRAY_SIZE(rt5610_reg),
 	.reg_word_size = sizeof(u16),
 	.reg_cache_step = 2,
 	.reg_cache_default = rt5610_reg,
 };
 
 static int rt5610_codec_probe(struct platform_device *pdev)
 {
 	return snd_soc_register_codec(&pdev->dev,
 			&soc_codec_dev_rt5610, rt5610_dai, ARRAY_SIZE(rt5610_dai));
 }
 
 static int rt5610_codec_remove(struct platform_device *pdev)
 {
 	snd_soc_unregister_codec(&pdev->dev);
 	return 0;
 }
 
 static const struct of_device_id rt5610_dt_ids[] = {
 	{ .compatible = "fsl,audio-rt5610", },
 	{ /* sentinel */ }
 };
 MODULE_DEVICE_TABLE(of, rt5610_dt_ids);
 
 static struct platform_driver rt5610_codec_driver = {
 	.driver = {
 		.name = "rt5610",
 		.owner = THIS_MODULE,
 		.of_match_table = rt5610_dt_ids,
 	},
 	.probe = rt5610_codec_probe,
 	.remove = rt5610_codec_remove,
 };
 module_platform_driver(rt5610_codec_driver);
 MODULE_DESCRIPTION("ASoC RT5610 driver");
 MODULE_AUTHOR("Johnny Hsu <johnnyhsu@realtek.com>");
 MODULE_LICENSE("GPL");
 
