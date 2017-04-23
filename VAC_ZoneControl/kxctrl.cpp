// kX kxctrl utility
// Copyright (c) Eugene Gavrilov, 2001-2005.
// All rights reserved

// This program is free software; you can redistribute it and/or
// modify it under the terms of the 
// EUGENE GAVRILOV KX DRIVER SOFTWARE DEVELOPMENT KIT LICENSE AGREEMENT

#include "stdafx.h"

#include "interface/kxapi.h"

#include "vers.h"

#pragma warning(disable:4100)

iKX *ikx=NULL;
int parse_file(char *file_name,int (__stdcall *command)(int argc,char **argv));
int parse_text(char *text,int size,int (__stdcall *command)(int argc,char **argv));
int batch_mode=0;

static int find_reg(word reg,dsp_register_info *info,int info_size)
{
 for(dword i=0;i<info_size/sizeof(dsp_register_info);i++)
 {
 	if(info[i].num==reg)
 	 return i;
 }
 return -1;
}

void disassemble(dword *d,int sz)
{
 dword d1,d2;
 while(sz>0)
 {
  d1=*d++;
  d2=*d++;
  sz-=8;

  word op,res,a,x,y;

  if(!ikx->get_dsp())
  {
  op=(word)((d2&E10K1_OP_MASK_HI)>>E10K1_OP_SHIFT_LOW);
  res=(word)((d2&E10K1_R_MASK_HI)>>E10K1_OP_SHIFT_HI);
  a=(word)((d2&E10K1_A_MASK_HI));
  x=(word)((d1&E10K1_X_MASK_LOW)>>E10K1_OP_SHIFT_HI);
  y=(word)((d1&E10K1_Y_MASK_LOW));
  }
  else
  {
  op=(word)((d2&E10K2_OP_MASK_HI)>>E10K2_OP_SHIFT_LOW);
  res=(word)((d2&E10K2_R_MASK_HI)>>E10K2_OP_SHIFT_HI);
  a=(word)((d2&E10K2_A_MASK_HI));
  x=(word)((d1&E10K2_X_MASK_LOW)>>E10K2_OP_SHIFT_HI);
  y=(word)((d1&E10K2_Y_MASK_LOW));
  }

  kString s;
  s="\t ";
  ikx->format_opcode(&s,op);
  s+=" \t";
  ikx->format_reg(&s,res,NULL,0);
  s+=", ";
  ikx->format_reg(&s,a,NULL,0);
  s+=", ";
  ikx->format_reg(&s,x,NULL,0);
  s+=", ";
  ikx->format_reg(&s,y,NULL,0);
  s+="; ";
//  ikx->format_opcode(&s,op,1);
  s+="\n";

  printf("%s",(LPCTSTR)s);
 }
}

void combine(char **argv)
{
  kString tmp;

  char name[KX_MAX_STRING];
  char copyright[KX_MAX_STRING];
  char engine[KX_MAX_STRING];
  char comment[KX_MAX_STRING];
  char created[KX_MAX_STRING];
  char guid[KX_MAX_STRING];

  int i;
  dsp_code *code=NULL;
  dsp_register_info *info=NULL;

  dsp_code *code2=NULL;

  dword m_dump[1024*2]; // code; 1024 or 512 x 2 dwords each instruction
  dword m_gprs[512];    // gpr: 256 or 512
  dword m_tram[256*2];  // tram: addr and data
  dword m_flg[256];     // 10k2 flg
  unsigned int n_instr=0,n_gprs=0;
  int is_10k2=0;

  int code_size=0,info_size=0,itramsize=0,xtramsize=0;
  int ret;
  FILE *f;
  f=fopen(argv[1],"rb");
  if(f)
  {
    fseek(f,0L,SEEK_END);
    size_t fsize=ftell(f);
    fseek(f,0L,SEEK_SET);
    char *mem=(char *)malloc(fsize);
    if(mem==NULL)
    {
     fclose(f);
     fprintf(stderr,"Not enough memory to load RIFX\n");
     return;
    }
    fsize=fread(mem,1,fsize,f);
    fclose(f);
    f=NULL;
    code=NULL;
    info=NULL;
    ret=ikx->parse_rifx(mem,fsize,name,&code,&code_size,
		&info,&info_size,&itramsize,&xtramsize,
		copyright,engine,created,comment,guid);
    free(mem);
    if(ret<0)
    {
    	fprintf(stderr,"Error parsing RIFX (%d)\n",ret);
    	goto END;
    }
    else
    {
    	fprintf(stderr,"RIFX successfuly parsed [%s]\n",name);
    	if(ret!=0)
    	 fprintf(stderr,"Warnings: %d\n",ret);
    }
  } // f!=NULL
  else
  {
  	fprintf(stderr,"Cannot open '%s'\n",argv[1]);
  	goto END;
  }

 f=fopen(argv[2],"rb"); // dump
 if(!f)
 {
   fprintf(stderr,"Error opening dump\n");
   goto END;
 }
 char buff[17];
 fread(buff,1,16,f);
 if(strncmp(buff,"10k1 microcode $",16)==NULL)
 {
  ikx->set_dsp(0);
  n_gprs=256;
  n_instr=512;
  is_10k2=0;
 }
 else
  if(strncmp(buff,"10k2 microcode $",16)==NULL)
  {
   ikx->set_dsp(1);
   n_gprs=512;
   n_instr=1024;
   is_10k2=1;
  }
  else
   {
    fprintf(stderr,"Incorrect dump\n");
    goto END;
   }

 code2=(dsp_code *)malloc(n_instr*sizeof(dsp_code));
 if(code2==NULL)
 {
  fprintf(stderr,"No mem\n");
  goto END;
 }

  dword magic;
  magic=0xdeadbeaf;
  fread(&magic,1,4,f);
  if(magic!='$rcm')
  {
   fprintf(stderr,"Possibly incorrect dump file! [no microcode signature; '%x']\n",magic);
   goto END;
  }

 if(fread(&m_dump[0],1*4*2,n_instr,f)!=n_instr)
 {
  fprintf(stderr,"Bad dump file [microcode]\n");
  goto END;
 }

         magic=0xdeadbeaf; 
         fread(&magic,1,4,f);
         if(magic!='$rpg')
         {
          fprintf(stderr,"Possibly incorrect dump file! [no gpr signature; '%x']\n",magic);
          goto END;
         }

 if(fread(&m_gprs[0],4,n_gprs,f)!=n_gprs)
 {
  fprintf(stderr,"Bad dump file - $gpr section\n");
  goto END;
 }

 magic=0xdeadbeaf;
 fread(&magic,1,4,f);
 if(magic!='$mrt')
 {
  fprintf(stderr,"Possibly incorrect dump file! [no tram signature; '%x']\n",magic);
  goto END;
 }

 if(fread(&m_tram[0],2*4,256,f)!=256) // tram data & addr
 {
  fprintf(stderr,"Bad dump file - $trm section\n");
  goto END;
 }

 if(is_10k2)
 {
         magic=0xdeadbeaf;
         fread(&magic,1,4,f);
         if(magic!='$glf')
         {
          fprintf(stderr,"Possibly incorrect dump file! [no flag signature; '%x']\n",magic);
          goto END;
         }

         if(fread(&m_flg[0],1,256,f)!=256)
         {
          fprintf(stderr,"Bad dump file - $flg section\n");
          goto END;
         }
 }
 fclose(f);
 f=NULL;

 fprintf(stderr,"Dump file read\n");

 dword d1,d2;
 for(i=0;i<(int)n_instr;i++)
 {
  d1=m_dump[i*2];
  d2=m_dump[i*2+1];

  int op,res,a,x,y;

  if(is_10k2)
  {
   op=(d2&E10K2_OP_MASK_HI)>>E10K2_OP_SHIFT_LOW;
   res=(d2&E10K2_R_MASK_HI)>>E10K2_OP_SHIFT_HI;
   a=(d2&E10K2_A_MASK_HI);
   x=(d1&E10K2_X_MASK_LOW)>>E10K2_OP_SHIFT_HI;
   y=(d1&E10K2_Y_MASK_LOW);
  }
  else
  {
   op=(d2&E10K1_OP_MASK_HI)>>E10K1_OP_SHIFT_LOW;
   res=(d2&E10K1_R_MASK_HI)>>E10K1_OP_SHIFT_HI;
   a=(d2&E10K1_A_MASK_HI);
   x=(d1&E10K1_X_MASK_LOW)>>E10K1_OP_SHIFT_HI;
   y=(d1&E10K1_Y_MASK_LOW);
  }
  code2[i].op=(byte)op;
  code2[i].r=(word)res;
  code2[i].a=(word)a;
  code2[i].x=(word)x;
  code2[i].y=(word)y;
 }

 fprintf(stderr,"code uploaded\n");

 // search for rifx 'code' in 'code2'
 dword j;
 int offset;
 offset=-1;
 
 for(i=0;i<(int)n_instr;i++)
 {
  if(code2[i].op==code[0].op) // test
  {
   for(j=0;j<code_size/sizeof(dsp_code);j++)
    if(code2[i+j].op!=code[j].op)
     break;
   if(j==code_size/sizeof(dsp_code))
   {
    offset=i;
    fprintf(stderr,"RIFX Found in dump: @ offset= %d\n",i);
    printf("; RIFX is located @ offset %d\n",i);
   }
  }
 }

 fprintf(stderr,"dump search completed [%d]\n",offset);
 
 if(offset==-1)
 {
  fprintf(stderr,"RIFX not found in the dump\n");
  goto END;
  }

 // now we have: code[0..] - rifx; code2[offset..] - dump in memory corresponding rifx
 // re-initialize gprs
 #define is_gpr(a) ((!is_10k2)?((a)>=0x100 && (a)<=0x1ff):((a)>=0x400 && (a)<=0x5ff))

 for(i=0;i<(int)(code_size/sizeof(dsp_code));i++)
 {
    int pos;
#define analyze_gpr(gpr,gpr2)	\
     if( ((!is_10k2) && (gpr>=0x100 && gpr<=0x3ff)) ||     \
         (is_10k2 && (gpr>=0x200 && gpr<=0x5ff))           \
       )			\
     {							\
     pos=find_reg((word)gpr2,info,info_size);		\
     if(pos==-1)					\
     {							\
      fprintf(stderr,"Error in RIFX: pointer (%xh) to nowhere [@%d]\n",gpr2,i);	\
      goto END;						\
     }							\
     /* found: translate it */				\
    if(is_gpr(gpr))					\
    {							\
         /*printf("kxctrl s 4 %x %x %x\n",gpr2,gprs[(gpr-0x100)*2],gprs[gpr]);*/	\
         info[pos].p=m_gprs[(gpr-(is_10k2?0x400:0x100))];			\
         info[pos].translated=gpr;			\
    }							\
    }

   analyze_gpr(code2[i+offset].r,code[i].r);
   analyze_gpr(code2[i+offset].a,code[i].a);
   analyze_gpr(code2[i+offset].x,code[i].x);
   analyze_gpr(code2[i+offset].y,code[i].y);
 }

 fprintf(stderr,"analysis done\n");

 ikx->disassemble_microcode(&tmp,KX_DISASM_DANE,-1,
   code,code_size,
   info,info_size,
   itramsize,xtramsize,
   name,copyright[0]?copyright:NULL,engine[0]?engine:NULL,created[0]?created:0,comment[0]?comment:0,
   guid[0]?guid:0);

 fprintf(stderr,"writing results\n");

 fwrite(tmp.GetBuffer(1),1,tmp.GetLength(),stdout);
 tmp.ReleaseBuffer();

END:
  if(code)
  {
   LocalFree(code);
   code=NULL;
   }
  if(info)
  {
   LocalFree(info);
   info=NULL;
  }
  if(code2)
  {
	  free(code2);
	  code2=NULL;
  }
  if(f)
  {
   fclose(f);
  }
}

void help(void)
{
   fprintf(stderr,
	" -ml <rifx file>\t\t - load RIFX/bin microcode\n"
	" -me <id>\t\t\t - enable microcode\n"
	" -mb <id> <mode>\t\t - toggle microcode bypass mode\n"
	" -gmf <id> \t\t\t - get microcode flag\n"
	" -smf <id> <flag>\t\t - set microcode flag\n"
	" -md <id>\t\t\t - disable microcode\n"
	" -mu <id>\t\t\t - unload microcode\n"
	" -mp [id]\t\t\t - list all uploaded effects / print fx regs\n"
	" -mo id\t\t\t\t - dump microcode (with opcodes)\n"
        " -mc <srcid> <reg> <dstid> <reg> - connect (reg is hex w/o '0x' or reg name)\n"
        " -mdc <srcid> <reg> \t\t - disconnect (reg is hex w/o '0x' or reg name)\n"
	"\t\t\t\t   if src | dst == -1, then reg is physical\n"
	" -ma <id> <pgm_name> <reg_name>\t - assign microcode slider to dsp pgm\n"
    "\t\t\t\t   [id: 0..5 - master/rec/kx0/kx1/synth/wave]\n"
    "\t\t\t\t   [set pgm_name to 'undefined'  to unmap]\n"
	" -mr\t\t\t\t - reset microcode to default\n"
	" -mrc\t\t\t\t - clear all microcode and stop the DSP\n"
	" -reset\t\t\t\t - reset settings to defaults\n"
	" -rv\t\t\t\t - reset hardware voices\n"
	" -dbr\t\t\t\t - re-initialize daughter board card(s)\n"
	" -mx <dll_name>\t\t\t - extract RIFX effects from DLL to .\\rifx\n"
    	" -da [<file.bin>]\t\t - disassemble current 10kx/file's microcode\n"
        " -co <rifx.rifx> <memdump.bin> \t - combine rifx & dump to stdout [10k1 only]\n"
        "\n");
   fprintf(stderr,"-- pause --\n");
   _getch();
   fprintf(stderr,
        "\n"
    	" -sg <pgm> <id> <val1> <val2>\t - set GPR\n"
    	" -gg <pgm> <id>\t\t\t - get GPR\n"
    	" -sr <id> <value>\t\t - set routing (value: hex - aaBBccDD)\n"
    	" -gr <id>\t\t\t - get routing (value: hex)\n"
    	" -sf <id> <value>\t\t - set FX amount (hex:0..ff)\n"
    	" -gf <id> <value>\t\t - get FX amount (hex:0..ff)\n"
    	" -shw <id> <value>\t\t - set HW parameter\n"
        " -ghw <id>\t\t\t - get HW parameter\n"
        " -istat\t\t\t\t - get spdif / i2s status\n"
        "\n"
    	" -dd <num>\t\t\t - get driver's dword value\n"
    	" -ds <num>\t\t\t - get driver's string value\n"
    	" -sdb <id> <val>\t\t - set driver's buffer value (hex)\n"
    	" -gdb <id>\t\t\t - get driver's buffer value (hex)\n"
    	" -{s|g}{ac97|ptr|fn0|p}\t\t - set HAL register [reg [ch] val]\n"
    	"\t\t\t\t (-sac97 0 0 - reset AC97)\n"
    	" -sf2 {l|c|p|i|u} {dir|file} [{file|dir}]\n"
    	"\t\t\t\t - Load/Compile/Parse/Info/Unload SoundFonts\n"
        " --nokx\t\t\t\t - do not init the driver (should be the last)\n"
        " --gui\t\t\t\t - perform interactive commands\n"
        " $<x>\t\t\t\t - use card number <x> (should be the first)\n"
    	);
 if(!batch_mode)
 {
  if(ikx)
  {
    delete ikx;
    ikx=NULL;
  }	
  exit(1);
 }
}

char *assignment_to_text(int i)
{
 switch(i)
 {
  case MIXER_MASTER: return "Master";
  case MIXER_WAVE: return "Wave";
  case MIXER_SYNTH: return "Synth";
  case MIXER_KX0: return "kX0";
  case MIXER_KX1: return "kX1";
  case MIXER_REC: return "Rec";
 }
 return "(undef)";
}

int print_info(void)
{
	int sz=-3;
       	sz=ikx->enum_soundfonts(0,0);
       	if(sz>0)
       	{
       	  printf("%d soundfonts [%d bytes]\n",sz/sizeof(sfHeader),sz);
       	  sfHeader *hdr;
       	  hdr=(sfHeader *)malloc(sz);
       	  if(hdr)
       	  {
       	   if(!ikx->enum_soundfonts(hdr,sz))
       	   {
       	     for(dword i=0;i<sz/sizeof(sfHeader);i++)
       	     {
       	      printf("[%d] '%s'\n",hdr[i].rom_ver.minor,&hdr[i].name[0]);
       	     }
       	   } else { printf("Error enumerating sf\n"); sz=-2; }
       	   free(hdr);
       	  } else { printf("Not enough memory (%d)\n",sz); sz=-1; }
       	} else
       	{
       	 if(sz==0)
       	  printf("No loaded soundfonts\n");
       	 else
       	  printf("Error occured while acquiring soundfonts\n");
       	}
       	return sz;
}


int _stdcall process(int argc, char **argv)
{
      	dword v1;
      	int pgm;
      	word id;
        char name[512];
        dsp_code *code;
        dsp_register_info *info;
        int code_size,info_size,itramsize,xtramsize;
        int ret;
        FILE *f=0;

        if(strcmp(argv[0],"--gui")==NULL)
        {
            fprintf(stderr,"Using device '%s'\n",ikx->get_device_name());
            printf("Entering interactive mode. Type '?' for help. Type 'quit' to exit\n\n");
            batch_mode=1;
            while(1)
            {
             char cmd[128];
             printf(">");
             gets(cmd);
             if(cmd[0]=='?')
             {
              help();
              printf("Type 'quit' to quit\n");
             }
             else
              if(strcmp(cmd,"quit")==0)
               break;
              else
              {
               char to_do[128];
               sprintf(to_do,"-%s",cmd);
               parse_text(to_do,strlen(to_do),process);
              }
            }
        }
        else
	if(strcmp(argv[0],"-gg")==NULL)
	{
   		if(argc<3)
   		   help();
   		  else
           	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
           	{
           	  if(sscanf(argv[2],"%x",&id)==1)
           	  {
           	   if(ikx->get_dsp_register(pgm,id,&v1))
           	    printf("Get GPR failed\n");
           	   else
           	    printf("got GPR = %x\n",v1);
           	  } else printf("Bad id\n");
           	} else printf("Bad pgm\n");
        }
        else
        if(strcmp(argv[0],"-sg")==NULL)
        {
   		if(argc<4)
   		   help();
   		  else
           	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
           	{
           	  if(sscanf(argv[2],"%x",&id)==1)
           	  {
           	   if(sscanf(argv[3],"%x",&v1)==1)
           	   {
            	    if(ikx->set_dsp_register(pgm,id,v1))
           	     printf("Get GPR failed\n");
           	    else
           	     printf("set GPR to = %x %x\n",v1);
           	   } else printf("Bad values\n");
           	  } else printf("Bad id\n");
           	} else printf("Bad pgm\n");
        }
        else
        if(strcmp(argv[0],"-ml")==NULL)
        {
      		 if(argc<2)
   		   help();
   		  else
   		  {
                 f=fopen(argv[1],"rb");
                 if(f)
                 {
                   fseek(f,0L,SEEK_END);
                   size_t fsize=ftell(f);
                   fseek(f,0L,SEEK_SET);
                   char *mem=(char *)malloc(fsize);
                   if(mem==NULL)
                   {
                    fclose(f);
                    printf("Not enough memory to load RIFX\n");
                    return -9;
                   }
                   fsize=fread(mem,1,fsize,f);
                   fclose(f);
                   code=NULL;
                   info=NULL;
                   char m_copyright[KX_MAX_STRING];
                   char m_engine[KX_MAX_STRING];
                   char m_created[KX_MAX_STRING];
                   char m_comment[KX_MAX_STRING];
                   char m_guid[KX_MAX_STRING];

                   ret=ikx->parse_rifx(mem,fsize,name,&code,&code_size,
               		&info,&info_size,&itramsize,&xtramsize,m_copyright,m_engine,m_created,m_comment,m_guid);
                   if(ret<0)
                   {
                   	fprintf(stderr,"Error parsing RIFX (%d)\n",ret);
                   }
                   else
                   {
                   	printf("RIFX successfuly parsed [%s] Warnings: %d\n",name,ret);
                   }
                   free(mem);

                   // now, translate & load
                   int pgm=ikx->load_microcode(name,code,code_size,info,info_size,itramsize,xtramsize,
                    m_copyright,m_engine,m_created,m_comment,m_guid);
                   if(pgm==0)
                   {
                    fprintf(stderr,"Load microcode failed\n");
                    }
                   else
                   {
                    printf("Microcode loaded; pgm=%d\n",pgm);
                    if(!ikx->translate_microcode(pgm))
                    {
                     printf("Microcode translated\n");
                    }
                    else
                    {
                     printf("Microcode translation failed\n");
                     ikx->unload_microcode(pgm);
                    }
                   }

                   if(code)
                   {
                    LocalFree(code);
                    code=NULL;
                    }
                   if(info)
                   {
                    LocalFree(info);
                    info=NULL;
                   }
                 } // f!=NULL
                 else perror("Error opening RIFX file");

                 }
        }
        else
        if(strcmp(argv[0],"-mu")==NULL)
        {
     		if(argc<2)
   		   help();
   		else
   		{
         	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
         	{
         		if(ikx->unload_microcode(pgm))
         		 printf("Unload microcode failed\n");
         		else
         		 printf("Microcode unloaded\n");
         	}
         	else
         	{
         		printf("Bad ID specified [%s]\n",argv[1]);
         	}
         	}
        }
        else
        if(strcmp(argv[0],"-mo")==NULL)
        {
     		if(argc<2)
   		   help();
   		 else
   		{
        	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
         	{
                         	dsp_microcode mc;
                         	if(ikx->enum_microcode(pgm,&mc)==0)
                         	{
                         	 fprintf(stderr,"[%d] %s (%s%s%s); code_size=%d info_size=%d\n",
                         	  pgm,
                         	  mc.name,
                         	  mc.flag&MICROCODE_TRANSLATED?"Uploaded ":"",
                         	  mc.flag&MICROCODE_ENABLED?"Enabled ":"",
                         	  mc.flag&MICROCODE_BYPASS?"ByPass":"",
                         	  mc.code_size,mc.info_size);
                         	 info=(dsp_register_info *)malloc(mc.info_size); memset(info,0,mc.info_size);
                         	 code=(dsp_code *)malloc(mc.code_size); memset(code,0,mc.code_size);
                         	 if(!ikx->get_microcode(pgm,code,mc.code_size,info,mc.info_size))
                         	 {
                         	  kString tmp;
                                  ikx->disassemble_microcode(&tmp,KX_DISASM_DANE,pgm,code,mc.code_size,info,mc.info_size,mc.itramsize,mc.xtramsize,mc.name,mc.copyright,mc.engine,mc.created,mc.comment,mc.guid);
                                  fwrite(tmp.GetBuffer(10),1,tmp.GetLength(),stdout);
                                  tmp.ReleaseBuffer();
                         	 } else printf("error getting microcode\n");
                         	 free(code);
                         	 free(info);
                         	} else printf("no such microcode\n");
         	}
         	else
         	{
         		printf("Bad ID specified [%s]\n",argv[1]);
         	}
         	}
        }
        else
        if(strcmp(argv[0],"-md")==NULL)
        {
     		if(argc<2)
   		   help();
   		else
   		{
         	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
         	{
         		if(ikx->disable_microcode(pgm))
         		 printf("Disable microcode failed\n");
         		else
         		 printf("Microcode disabled\n");
         	}
         	else
         	{
         		printf("Bad ID specified [%s]\n",argv[1]);
         	}
         	}
        }
        else
        if(strcmp(argv[0],"-me")==NULL)
        {
     		if(argc<2)
   		   help();
   		 else
   		 {

        	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
        	{
        		if(ikx->enable_microcode(pgm))
        		 printf("Enable microcode failed\n");
        		else
        		 printf("Microcode enabled\n");
        	}
        	else
        	{
        		printf("Bad ID specified [%s]\n",argv[1]);
        	}
        	}
        }
        else
        if(strcmp(argv[0],"-smf")==NULL)
        {
     		if(argc<3)
   		   help();
   		 else
   		 {
        	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
        	{
        	 dword state=0;
                 sscanf(argv[2],"%x",&state);
       		 if(ikx->set_microcode_flag(pgm,state))
       		  printf("Microcode flag change failed\n");
       		 else
       		  printf("Microcode flag changed to '%x'\n",state);
        	}
        	else
        	{
        		printf("Bad ID specified [%s]\n",argv[1]);
        	}
        	}
        }
        else
        if(strcmp(argv[0],"-gmf")==NULL)
        {
     		if(argc<2)
   		   help();
   		 else
   		 {
        	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
        	{
        	 dword state=0;
       		 if(ikx->get_microcode_flag(pgm,&state))
       		  printf("Error getting microcode flag\n");
       		 else
       		  printf("Microcode [%d] flag: '%x'\n",pgm,state);
        	}
        	else
        	{
        		printf("Bad ID specified [%s]\n",argv[1]);
        	}
        	}
        }
        else
        if(strcmp(argv[0],"-mb")==NULL)
        {
     		if(argc<3)
   		   help();
   		 else
   		 {
        	if(sscanf(argv[1],"%d",&pgm)==1) // id is #
        	{
        	 int state=0;
                 sscanf(argv[2],"%d",&state);
       		 if(ikx->set_microcode_bypass(pgm,state))
       		  printf("Microcode bypass mode switch failed\n");
       		 else
       		  printf("Microcode bypass mode set to '%s'\n",state?"On":"Off");
        	}
        	else
        	{
        		printf("Bad ID specified [%s]\n",argv[1]);
        	}
        	}
        }
        else
        if(strcmp(argv[0],"-reset")==NULL) // reset settings
        {
        	if(ikx->reset_settings())
        	 printf("Error resetting settings\n");
        	else
        	 printf("Reset settings succeeded\n");
        }
        else
        if(strcmp(argv[0],"-rv")==NULL) // reset settings
        {
        	if(ikx->reset_voices())
        	 printf("Error resetting hw voices\n");
        	else
        	 printf("Reset hw voices succeeded\n");
        }
        else
        if(strcmp(argv[0],"-dbr")==NULL) // reset dB
        {
        	if(ikx->reset_db())
        	 printf("Error resetting db\n");
        	else
        	 printf("DB successfully reset\n");
        }
        else
        if(strcmp(argv[0],"-mr")==NULL)
        {
        	if(ikx->reset_microcode())
        	{
        		printf("Reset failed\n");
        	}
        	else
        	{
        		printf("Reset succeeded\n");
        	}
        }
        else
        if(strcmp(argv[0],"-mrc")==NULL)
        {
        	if(ikx->dsp_clear())
        	{
        		printf("DSP Clear failed\n");
        	}
        	else
        	{
        		printf("DSP Clear succeeded\n");
        	}
        }
        else
        if(strcmp(argv[0],"-mx")==NULL)
        {
	   if(argc<2)
  	     help();
  	    else
  	   {

           f=fopen(argv[1],"rb");
           if(f)
           {
             fseek(f,0L,SEEK_END);
             size_t fsize=ftell(f);
             fseek(f,0L,SEEK_SET);
             char *mem=(char *)malloc(fsize);
             if(mem==NULL)
             {
              fclose(f);
              printf("Not enough memory to load RIFX\n");
              return -9;
             }
             fsize=fread(mem,1,fsize,f);
             fclose(f);

             size_t pos=0;
             CreateDirectory("rifx",NULL);
             _chdir("rifx");

             #pragma warning(disable:4127)
             while(1)
             {
             		if(strncmp(&mem[pos],"RIFX",4)==NULL)
             		{
             		    char copyright[KX_MAX_STRING];
             		    char engine[KX_MAX_STRING];
             		    char created[KX_MAX_STRING];
             		    char comment[KX_MAX_STRING];
             		    char guid[KX_MAX_STRING];

                             code=NULL;
                             info=NULL;
                             ret=ikx->parse_rifx(&mem[pos],fsize-pos,name,&code,&code_size,
                         		&info,&info_size,&itramsize,&xtramsize,
                         		&copyright[0],
                         		&engine[0],
                         		&created[0],
                         		&comment[0],
                         		guid);
                             if(ret<0)
                             {
                             	printf("Error parsing RIFX (%d)\n",ret);
                             }
                             else
                             {
                             	printf("RIFX successfuly parsed [%s] warnings: %d\n",name,ret);

                                ikx->generate_guid(guid);

                                char tmp_name[KX_MAX_STRING+4];
                                strcpy(tmp_name,name);
                                strcat(tmp_name,".da");
                             	
                             	f=fopen(tmp_name,"wt");
                             	if(f)
                             	{
                             	    kString tmp;
                             	    ikx->disassemble_microcode(&tmp,KX_DISASM_REGS|KX_DISASM_CODE,-1,code,code_size,info,info_size,itramsize,xtramsize,
                             	     name,
                             	     copyright[0]?copyright:0,
                             	     engine[0]?engine:0,
                             	     created[0]?created:0,
                             	     comment[0]?comment:0,
                             	     guid[0]?guid:0);
                                    fwrite(tmp.GetBuffer(10),1,tmp.GetLength(),f);
                                    tmp.ReleaseBuffer();
                             	    fclose(f);
                             	}
                             	strcat(name,".rifx");
                             	f=fopen(name,"wb");
                             	if(f)
                             	{
                             	    dword szz=(byte)mem[pos+7]+(((byte)mem[pos+6])<<8)+8;
                             	    fwrite(&mem[pos],szz,1,f);
                                    // write GUID here
                                    fprintf(f,"$kX$\nguid=%s\ngenerated by kX Ctrl Utility version %x\n",guid,KX_VERSION_DWORD);
                             	    fclose(f);
                             	}
                             }
                            if(code)
                            {
                             LocalFree(code);
                             code=NULL;
                             }
                            if(info)
                            {
                             LocalFree(info);
                             info=NULL;
                            }
                         }

                         pos+=4;
                         if(pos>=fsize)
                          break;
             }
             _chdir("..");
             free(mem);
          } else perror("Error loading file");
          }
        }
        else
        if(strcmp(argv[0],"-da")==NULL)
        {
                if(argc==1) // current
                {
                	if(!ikx->get_dsp()) 
                	{
                        	dword microcode[1024];
                                for(dword i=0;i<1024;i++)
                                 ikx->ptr_read(E10K1_MICROCODE_BASE+i,0,&microcode[i]);
                                disassemble(microcode,1024*4);
                        }
                        else // 10k2
                        {
                        	dword microcode[2048];
                                for(dword i=0;i<2048;i++)
                                 ikx->ptr_read(E10K2_MICROCODE_BASE+i,0,&microcode[i]);
                                disassemble(microcode,2048*4);
                        }
                }
                else // file
                {
     			if(argc<2)
   		   	   help();
   		   	 else
   		   	 {

                	FILE *f;
                	f=fopen(argv[1],"rb");
                	if(f)
                	{
                          fseek(f,0L,SEEK_END);
                          size_t fsize=ftell(f);
                          fseek(f,0L,SEEK_SET);
                          char *mem=(char *)malloc(fsize);
                          if(mem==NULL)
                          {
                           fclose(f);
                           printf("Not enough memory to load file\n");
                           return -9;
                          }
                          fsize=fread(mem,1,fsize,f);

                	  fclose(f);
                	  if(memcmp(mem,"10k1 microcode $",16)==0)
                	  {
                	   ikx->set_dsp(0);
                           disassemble((dword *)(mem+16+4),512*4*2);
                          }
                          else
                           if(memcmp(mem,"10k2 microcode $",16)==0)
                           {
                            ikx->set_dsp(1);
                            disassemble((dword *)(mem+16+4),1024*4*2);
                           }
                           else
                            printf("Incorrect microcode dump file\n");
                          free(mem);
                        } else perror("Error opening file");
                        }
                }
        }
        else
        if(strcmp(argv[0],"-mp")==NULL)
        {
                printf("// Microcode state:\n");
                if(argc==1)
                {
                     int i;
                     for(i=0;i<MAX_PGM_NUMBER;i++)
                     {
                     	dsp_microcode mc;
                     	if(ikx->enum_microcode(i,&mc)==0)
                     	{
                     	 printf("[%d] %s (%s%s%s) - %s\n",i,mc.name,
                     	  mc.flag&MICROCODE_TRANSLATED?"Uploaded ":"",
                     	  mc.flag&MICROCODE_ENABLED?"Enabled ":"",
                     	  mc.flag&MICROCODE_BYPASS?"Bypass":"",
                     	  mc.guid);
                     	}
                     }
                }
                else
                {
     			if(argc<2)
   		   		help();
   		   	else
   		   	{	

                	if(sscanf(argv[1],"%d",&pgm)==1)
                	{
                         	dsp_microcode mc;
                         	if(ikx->enum_microcode(pgm,&mc)==0)
                         	{
                         	 printf("[%d] %s (%s%s%s); code_size=%d info_size=%d\n",
                         	  pgm,
                         	  mc.name,
                         	  mc.flag&MICROCODE_TRANSLATED?"Uploaded ":"",
                         	  mc.flag&MICROCODE_ENABLED?"Enabled ":"",
                         	  mc.flag&MICROCODE_BYPASS?"Bypass":"",
                         	  mc.code_size,mc.info_size);
                         	 info=(dsp_register_info *)malloc(mc.info_size); memset(info,0,mc.info_size);
                         	 code=(dsp_code *)malloc(mc.code_size); memset(code,0,mc.code_size);
                         	 if(!ikx->get_microcode(pgm,code,mc.code_size,info,mc.info_size))
                         	 {
                         	  kString tmp;
                                  ikx->disassemble_microcode(&tmp,KX_DISASM_VALUES,pgm,code,mc.code_size,info,mc.info_size,mc.itramsize,mc.xtramsize,mc.name,mc.copyright,mc.engine,mc.created,mc.comment,mc.guid);
                                  fwrite(tmp.GetBuffer(10),1,tmp.GetLength(),stdout);
                                  tmp.ReleaseBuffer();
                         	 } else printf("error getting microcode\n");
                         	 free(code);
                         	 free(info);
                         	} else printf("no such microcode\n");
                        }
                        }
                }
        }
        else
        if(strcmp(argv[0],"-mc")==NULL)
        {
     		if(argc<5)
   		   help();
   		 else
   		 {

                int pg1,pg2,id1,id2;
                pg1=0,pg2=0,id1=-1,id2=-1;
                sscanf(argv[1],"%d",&pg1);
                sscanf(argv[3],"%d",&pg2);
                if(sscanf(argv[2],"%x",&id1)==1 &&
                   sscanf(argv[4],"%x",&id2)==1)
                   {
                    if(ikx->connect_microcode(pg1,(word)id1,pg2,(word)id2))
                     printf("Connect failed\n");
                    else
                     printf("Connect ok\n");
                   }
                   else
                    printf("Only digits for now supported\n");
                  }
        }
        else
        if(strcmp(argv[0],"-ma")==NULL)
        {
     		if(argc<6)
     		{
     		 if(argc!=2)
   		      help();
             else
             {
     		    kx_assignment_info ai;
     		    sscanf(argv[1],"%d",&ai.level);
     		    if(ikx->get_dsp_assignments(&ai)==0)
     		    {
     		     printf("Level %d (%s) assigned to microcode pgm: '%s', registers: '%s'/'%s'; max: %x\n",
                 ai.level,assignment_to_text(ai.level),
                 ai.pgm, ai.reg_left,ai.reg_right,ai.max_vol);
     		    } else fprintf(stderr,"Error getting DSP assignments\n");
     		 }
   		    }
            else
            {
         		    kx_assignment_info ai;

                    char pgm[64],reg_left[64],reg_right[64];
                    int max_vol=0x7fffffff;

                    sscanf(argv[1],"%d",&id);
                    sscanf(argv[2],"%s",pgm);
                    sscanf(argv[3],"%s",reg_left);
                    sscanf(argv[4],"%s",reg_right);
                    sscanf(argv[5],"%x",&max_vol);

                    if(strcmp(pgm,"undefined")==NULL)
                    {
                     pgm[0]=0; reg_left[0]=0; reg_right[0]=0;
                    }

                    ai.level=id;
                    strcpy(ai.pgm,pgm);
                    strcpy(ai.reg_left,reg_left);
                    strcpy(ai.reg_right,reg_right);
                    ai.max_vol=max_vol;

                     if(ikx->set_dsp_assignments(&ai)==0)
                     {
                      printf("Level %d (%s) assigned to microcode id: pgm: '%s', registers: '%s'/'%s'; max: %x\n",
                        id,assignment_to_text(id),
                        ai.pgm, ai.reg_left,ai.reg_right,ai.max_vol);
                     } else fprintf(stderr,"Error settings DSP assignments\n");
                 }
        }
        else
        if(strcmp(argv[0],"-mdc")==NULL)
        {
     		if(argc<3)
   		   help();
   		 else
   		 {

                int pg1,id1;
                pg1=0,id1=-1;
                sscanf(argv[1],"%d",&pg1);
                if(sscanf(argv[2],"%x",&id1)==1)
                   {
                    if(ikx->disconnect_microcode(pg1,(word)id1))
                     printf("Disconnect failed\n");
                    else
                     printf("Disconnect ok\n");
                   }
                   else
                    printf("Only digits for now supported\n");
                 }
        }
        else
        if(strcmp(argv[0],"-co")==NULL)
        {
     		if(argc<3)
   		   help();
   		 else
        	combine(argv);
        }
        else
        if(strcmp(argv[0],"-sf")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	int n;
        	byte val;

        	if(sscanf(argv[1],"%d",&n)==1 && sscanf(argv[2],"%x",&val)==1)
        	{
        		if(!ikx->set_send_amount(n,val))
        		 printf("FX Amount for %d set to: %x\n",n,val);
        		else
        		 printf("Error setting FX amount\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-gf")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
                byte amount;

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_send_amount(n,&amount))
        		 printf("FX Amount for %d is: %x\n",n,amount);
        		else
        		 printf("Error getting FX amount\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-shw")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	int n;
        	dword val;

        	if(sscanf(argv[1],"%d",&n)==1 && sscanf(argv[2],"%x",&val)==1)
        	{
        		if(!ikx->set_hw_parameter(n,val))
        		 printf("HW Parameter %d set to: %x\n",n,val);
        		else
        		 printf("Error setting HW parameter\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-ghw")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
                dword amount;

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_hw_parameter(n,&amount))
        		 printf("HW Parameter %d is: %x\n",n,amount);
        		else
        		 printf("Error getting HW parameter\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-sr")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	int n;
        	dword val;

        	if(sscanf(argv[1],"%d",&n)==1 && sscanf(argv[2],"%x",&val)==1)
        	{
        		if(!ikx->set_routing(n,val)) // FIXME: xrouting
        		 printf("Routing for %d set to: %x\n",n,val);
        		else
        		 printf("Error setting routing\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-gr")==NULL) // FIXME: xrouting
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
                dword routing;

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_routing(n,&routing))
        		 printf("FX Amount for %d is: %x\n",n,routing);
        		else
        		 printf("Error getting routing\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-sdb")==NULL)
        {
        	if(argc<3)
        	 help();
        	{
        	int n;
        	int val;

        	if(sscanf(argv[1],"%d",&n)==1 && sscanf(argv[2],"%x",&val)==1)
        	{
        		if(!ikx->set_buffers(n,val))
        		 printf("Buffers for %d set to: %x\n",n,val);
        		else
        		 printf("Error setting buffers\n");
        	}
        	else help();
        	}	
        }
        else
        if(strcmp(argv[0],"-gdb")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
                int val;

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_buffers(n,&val))
        		 printf("Buffers for %d are: %x\n",n,val);
        		else
        		 printf("Error getting buffers\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-sac97")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	byte reg;
        	word val;
        	if((sscanf(argv[1],"%x",&reg)==1) && (sscanf(argv[2],"%x",&val)==1))
        	{
        		if(!ikx->ac97_write(reg,val))
        		 printf("wrote AC97: reg=%x val=%x\n",reg,val);
        		else
        		 printf("error writing AC97: reg=%x val=%x\n",reg,val);
        	}
        	else help();
         	}	
	}
        else
        if(strcmp(argv[0],"-gac97")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	byte reg;
        	word val;
        	if(sscanf(argv[1],"%x",&reg)==1)
        	{
        		if(!ikx->ac97_read(reg,&val))
        		 printf("AC97[reg=%x]=%x\n",reg,val);
        		else
        		 printf("error reading AC97 reg=%x\n",reg);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-sptr")==NULL)
        {
        	if(argc<4)
        	 help();
        	else
        	{
        	dword reg,ch,val;
        	if((sscanf(argv[1],"%x",&reg)==1) && (sscanf(argv[2],"%x",&ch)==1) &&
        	   (sscanf(argv[3],"%x",&val)==1))
        	{
        		if(!ikx->ptr_write(reg,ch,val))
        		 printf("wrote PTR: reg=%x ch=%x val=%x\n",reg,ch,val);
        		else
        		 printf("error writing PTR: reg=%x ch=%x val=%x\n",reg,ch,val);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-gptr")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	dword reg,ch,val;
        	if((sscanf(argv[1],"%x",&reg)==1) && (sscanf(argv[2],"%x",&ch)==1))
        	{
        		if(!ikx->ptr_read(reg,ch,&val))
        		 printf("PTR[reg=%x; ch=%x]=%x\n",reg,ch,val);
        		else
        		 printf("error reading PTR reg=%x\n",reg);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-sfn0")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	int reg,val;
        	if((sscanf(argv[1],"%x",&reg)==1) && (sscanf(argv[2],"%x",&val)==1))
        	{
        		if(!ikx->fn0_write(reg,val))
        		 printf("wrote fn0: reg=%x val=%x\n",reg,val);
        		else
        		 printf("error writing fn0: reg=%x val=%x\n",reg,val);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-gfn0")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	dword reg,val;
        	if(sscanf(argv[1],"%x",&reg)==1)
        	{
        		if(!ikx->fn0_read(reg,&val))
        		 printf("Fn0[reg=%x]=%x\n",reg,val);
        		else
        		 printf("error reading FN0 reg=%x\n",reg);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-sp")==NULL)
        {
        	if(argc<3)
        	 help();
        	else
        	{
        	int reg,val;
        	if((sscanf(argv[1],"%x",&reg)==1) && (sscanf(argv[2],"%x",&val)==1))
        	{
        		if(!ikx->p16v_write(reg,val))
        		 printf("wrote p16v: reg=%x val=%x\n",reg,val);
        		else
        		 printf("error writing p16v: reg=%x val=%x\n",reg,val);
        	}
        	else help();
        	}
	}
        else
        if(strcmp(argv[0],"-gp")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	dword reg,val;
        	if(sscanf(argv[1],"%x",&reg)==1)
        	{
        		if(!ikx->p16v_read(reg,&val))
        		 printf("p16V[reg=%x]=%x\n",reg,val);
        		else
        		 printf("error reading P16V reg=%x\n",reg);
        	}
        	else help();
        	}
	}
	else
	if(strcmp(argv[0],"-sf2")==NULL)
	{
		if(argc<2)
 		 help();
 		else
 		{

		switch(argv[1][0])
		{
		 case 'l':
		 case 'L':
			if(argc<3)
		 		help();
        		FILE *f;
        		f=fopen(argv[2],"rb");
        		if(f)
        		{
        		 fclose(f);
        		 ret=ikx->parse_soundfont(argv[2],NULL);
        		}
        		else
        		 ret=ikx->compile_soundfont(argv[2],NULL);
        		if(ret<=0)
        		 printf("Error: %d\n",ret);
        		else
        		 printf("SoundFont uploaded successfully. Id=%d\n",ret);
		 	break;
		 case 'c':
		 case 'C':
			if(argc<4)
		 		help();
		 	ret=ikx->compile_soundfont(argv[2],argv[3]);
        		if(ret)
        		 printf("Error: %d\n",ret);
        		else
        		 printf("Ok\n");
		 	break;
		 case 'p':
		 case 'P':
			if(argc<4)
		 		help();
                        // -------
#if 0
                        {
                         FILE *t;
                         t=fopen(argv[2],"rb");
                         char *m=(char *)malloc(26761242);
                         fread(m,1,26761242,t);
                         fclose(t);
                         char tt[32];
                         sprintf(tt,"mem://%x",m);

                         ret=ikx->parse_soundfont(tt,argv[3]);

                         free(m);
		 	}
#else
		 	ret=ikx->parse_soundfont(argv[2],argv[3]);
#endif
        		if(ret)
        		 printf("Error: %d\n",ret);
        		else
        		 printf("Ok\n");
		 	break;
		 case 'i':
		 case 'I':
		 	print_info();
		 	break;
		 case 'u':
		 case 'U':
		 	if(argc<3)
		 		help();
		 	int id;
		 	if(sscanf(argv[2],"%d",&id)==1)
		 	{
		 	  if(ikx->unload_soundfont(id))
		 	   printf("Error unloading\n");
		 	  else
		 	   printf("SoundFont unloaded\n");
		 	} else help();
		 	break;
		 default:
		 	help();
		}
		}
	}
        else
        if(strcmp(argv[0],"-dd")==NULL) // driver's dword
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
                dword val;

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_dword(n,&val))
        		 printf("dword value %d is: %x\n",n,val);
        		else
        		 printf("Error getting dword value\n");
        	}
        	else help();
        	}
        }
        else
        if(strcmp(argv[0],"-istat")==NULL)
        {
        	kx_spdif_i2s_status st;
        	if(!ikx->get_spdif_i2s_status(&st))
        	{
        	  printf("Status:\n"
        	   "spdif A: %x %x (%x)\n",st.spdif.channel_status_a,st.spdif.srt_status_a,st.spdif.channel_status_a_x);
        	  printf("spdif B: %x %x (%x)\n",st.spdif.channel_status_b,st.spdif.srt_status_b,st.spdif.channel_status_b_x);
        	  printf("spdif C: %x %x (%x)\n",st.spdif.channel_status_c,st.spdif.srt_status_c,st.spdif.channel_status_c_x);
        	  printf("i2s 0: %x\n",st.i2s.srt_status_0);
        	  printf("i2s 1: %x\n",st.i2s.srt_status_1);
        	  printf("i2s 2: %x\n",st.i2s.srt_status_2);
        	  printf("spdif 0: %x / %x\n",st.spdif.scs0,st.spdif.scs0x);
        	  printf("spdif 1: %x / %x\n",st.spdif.scs1,st.spdif.scs1x);
        	  printf("spdif 2: %x / %x\n",st.spdif.scs2,st.spdif.scs2x);
        	  printf("spdif freq: %d\n",st.spdif.spo_sr==0?44100:st.spdif.spo_sr==1?48000:96000);
        	  printf("p16v rec: %x\n",st.p16v);
        	} else printf("Error getting spdif / i2s status\n");
        }
        else
        if(strcmp(argv[0],"-ds")==NULL)
        {
        	if(argc<2)
        	 help();
        	else
        	{
        	int n;
        	char name[KX_MAX_NAME];

        	if(sscanf(argv[1],"%d",&n)==1)
        	{
        		if(!ikx->get_string(n,name))
        		 printf("Driver string for %d is: %s\n",n,name);
        		else
        		 printf("Error getting driver string\n");
        	}
        	else help();
        	}
        }
        else
        {
         if(!batch_mode)
          help();
         else
          fprintf(stderr,"Invalid command\n");
        }
   return 0;
}

int main(int argc, char* argv[])
{
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		fprintf(stderr,"Fatal Error: MFC initialization failed\n");
		return 1;
	}

   fprintf(stderr,"kX Driver Control Program\n"KX_COPYRIGHT_STR"\n");

   ikx=new iKX();
   if(!ikx)
   {
   	fprintf(stderr,"Error: No memory for iKX interface\n");
   	return -1;
   }
   int id=0;

   if(argc>1 && argv[1][0]=='$')
   { 
    id=argv[1][1]-'0'; 
    argc--; 
    argv++; 
   }

   int ret=ikx->init(id);
   if(ret)
   {
   	if(strstr(GetCommandLine(),"--nokx")==0)
   	{
   	 fprintf(stderr,"Error initializing iKX interface (%d - %s)\n",ret,ikx->get_error_string());
   	 delete ikx;
   	 return -2;
   	}
   }

   if(argc<2)
   	help();

   if(argv[1][0]=='@') // list of commands
   {
   	batch_mode=1;
   	parse_file(&argv[1][1],process);
   }
   else
   {
   	process(argc-1,&argv[1]);
   }

   ikx->close();
   delete ikx;
   return 0;
}
