#ifndef FLEX_ELF_H_
# define FLEX_ELF_H_

char *_flex_elf_get_buildid(const char *filename);
void *_flex_elf_get_sym(const char *filename, const char *fname);

#endif /* FLEX_ELF_H_ */
