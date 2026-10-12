// (AI-assisted) Exercise the exact daemon validator under ASan/UBSan.
#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static int safe_elf(const uint8_t* b,size_t n){
 if(n<sizeof(Elf64_Ehdr))return -1;
 const Elf64_Ehdr* h=(const void*)b;
 if(memcmp(b,"\177ELF",4)||h->e_ident[EI_CLASS]!=ELFCLASS64||h->e_ident[EI_DATA]!=ELFDATA2LSB||h->e_machine!=EM_X86_64||h->e_phentsize!=sizeof(Elf64_Phdr)||!h->e_phnum||h->e_phnum>128)return -1;
 if(h->e_phoff>n || h->e_phnum>(n-h->e_phoff)/sizeof(Elf64_Phdr))return -1;
 if(h->e_shoff>n || h->e_shnum>(n-h->e_shoff)/sizeof(Elf64_Shdr))return -1;
 uint64_t high=0;int entry=0;
 for(unsigned i=0;i<h->e_phnum;i++){
  const Elf64_Phdr* p=(const void*)(b+h->e_phoff+i*sizeof(*p));
  if(p->p_offset>n||p->p_filesz>n-p->p_offset||p->p_filesz>p->p_memsz||p->p_vaddr>128*1024*1024||p->p_memsz>128*1024*1024-p->p_vaddr)return -1;
  if(p->p_type==PT_LOAD){if(p->p_vaddr+p->p_memsz>high)high=p->p_vaddr+p->p_memsz;if((p->p_flags&PF_X)&&h->e_entry>=p->p_vaddr&&h->e_entry-p->p_vaddr<p->p_memsz)entry=1;}
 }
 if(h->e_type!=ET_DYN || !high || !entry)return -1;
 for(unsigned i=0;i<h->e_shnum;i++){
  const Elf64_Shdr* sh=(const void*)(b+h->e_shoff+i*sizeof(*sh));
  if(sh->sh_type!=SHT_NOBITS&&(sh->sh_offset>n||sh->sh_size>n-sh->sh_offset))return -1;
  if(sh->sh_type==SHT_RELA){if(sh->sh_size%sizeof(Elf64_Rela))return -1;for(size_t j=0;j<sh->sh_size/sizeof(Elf64_Rela);j++){
   const Elf64_Rela* r=(const void*)(b+sh->sh_offset+j*sizeof(*r));
   if((uint32_t)r->r_info==R_X86_64_RELATIVE && (r->r_offset>high||sizeof(uint64_t)>high-r->r_offset))return -1;
  }}
 }
 return 0;
}

int main(int argc,char** argv){
 if(argc!=2)return 1;FILE* f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);uint8_t* b=malloc(n);if(fread(b,1,n,f)!=n)return 3;fclose(f);
 if(safe_elf(b,n)!=0)return 4;
 for(size_t i=0;i<sizeof(Elf64_Ehdr);i++)if(safe_elf(b,i)==0)return 5;
 Elf64_Ehdr original=*(Elf64_Ehdr*)b;
 ((Elf64_Ehdr*)b)->e_phoff=UINT64_MAX;if(safe_elf(b,n)==0)return 6;*(Elf64_Ehdr*)b=original;
 ((Elf64_Ehdr*)b)->e_shoff=UINT64_MAX;if(safe_elf(b,n)==0)return 7;*(Elf64_Ehdr*)b=original;
 ((Elf64_Ehdr*)b)->e_entry=UINT64_MAX;if(safe_elf(b,n)==0)return 8;*(Elf64_Ehdr*)b=original;
 free(b);puts("ELF validation boundaries passed (AI-assisted)");return 0;
}
