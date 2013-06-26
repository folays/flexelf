#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>

#include <gelf.h>

/* USEFULL:
 * - http://stackoverflow.com/questions/1118705/call-a-function-named-in-a-string-variable-in-c
 * - http://us.generation-nt.com/answer/rfc-0-5-perf-tools-minimalistic-build-without-libelf-dependency-help-207440901.html
 * SOMEWHAT USEFULL:
 * - http://kerneldox.com/kdox-linux-2.6/d6/d3d/tools_2perf_2util_2symbol_8c_source.html
 * - http://freemanu1.free.fr/elf_examples/index.html
 * - http://freemanu1.free.fr/elf_examples/html/elf_copy.c.html
 * NOT READ:
 * - http://sourceforge.net/projects/elftoolchain/files/Documentation/libelf-by-example/
 */

struct flex_elf
{
  int fd;
  Elf *elf;
};

struct flex_elf_sym
{
  const char *name;
  void (*f)();
};

typedef void (f_cb_section)(struct flex_elf *fe, Elf_Scn *scn, void *udata);

#define DEBUG if (0)

#define ALIGN_NEXT(ptr, align) ((void *)(((uintptr_t)ptr) + (((align) - 1)) & ~(align - 1)))

static void _flex_elf_open(const char *filename, struct flex_elf *fe)
{
  fe->fd = open(filename, O_RDONLY);
  if (fe->fd < 0)
    err(1, "%s : open %s", __func__, filename);

  elf_version(EV_CURRENT);

  fe->elf = elf_begin(fe->fd, ELF_C_READ, NULL);
  if (!fe->elf)
    errx(1, "%s : elf_begin %s", __func__, elf_errmsg(elf_errno()));
}

static void _flex_elf_close(struct flex_elf *fe)
{
  elf_end(fe->elf);
  close(fe->fd);
}

static void _flex_elf_map_section(struct flex_elf *fe, Elf64_Word section,
				  f_cb_section *_f_cb_section, void *udata)
{
  Elf64_Ehdr *ehdr;
  Elf64_Shdr *shdr;
  Elf_Scn *scn;
  Elf_Data *data;
  int cnt;

  /* Let's get the elf sections */
  if (((ehdr = elf64_getehdr(fe->elf)) == NULL) ||
      ((scn = elf_getscn(fe->elf, ehdr->e_shstrndx)) == NULL) ||
      ((data = elf_getdata(scn, NULL)) == NULL)) {
    fprintf(stderr, "Failed to get SOMETHING\n");
    exit(1);
  }

  /* Let's go through each elf section looking for the symbol table */
  for (cnt = 1, scn = NULL; scn = elf_nextscn(fe->elf, scn); cnt++) {
    if ((shdr = elf64_getshdr(scn)) == NULL)
      exit(1);

    if (shdr->sh_type == section)
      {
	_f_cb_section(fe, scn, udata);
      }
  }  
}

static void _flex_mapper_buildid_section(struct flex_elf *fe, Elf_Scn *scn, void *udata)
{
  unsigned char **buildid = udata;

  Elf_Data *data = elf_getdata(scn, NULL);

  if (!data || !data->d_size)
    return;

  DEBUG printf("\e[32mNOTE DATA\e[0m offset %p size %lx align %ld\n", data->d_buf, data->d_size, data->d_align);

  Elf64_Nhdr *nhdr = data->d_buf;

  if (nhdr->n_type != NT_GNU_BUILD_ID)
    return;

  const char *note_name = (char *)(nhdr +1);
  if (nhdr->n_namesz != sizeof("GNU") || memcmp(note_name, "GNU", nhdr->n_namesz))
    return;

  const char *elf_buildid = ALIGN_NEXT(note_name + nhdr->n_namesz, data->d_align);

  *buildid = malloc(nhdr->n_descsz * 2 + 1);

  ptrdiff_t diff = 0;
  while (diff < nhdr->n_descsz)
    {
      sprintf(&(*buildid)[diff * 2], "%08x", *(uint32_t *)&elf_buildid[diff]);
      diff += sizeof(uint32_t);
    }

  (*buildid)[nhdr->n_descsz * 2] = '\0';
}

static void _flex_mapper_sym_section(struct flex_elf *fe, Elf_Scn *scn, void *udata)
{
  struct flex_elf_sym *fes = udata;

  Elf_Data *data = elf_getdata(scn, NULL);

  if (!data || !data->d_size)
    return;

  Elf64_Sym *sym;
  for (sym = data->d_buf; sym < (Elf64_Sym *)data->d_buf + data->d_size; ++sym)
    {
      if (!sym->st_value ||
	  (ELF64_ST_BIND(sym->st_info) == STB_WEAK) ||
	  (ELF64_ST_BIND(sym->st_info) == STB_NUM) ||
	  (ELF64_ST_TYPE(sym->st_info) != STT_FUNC)
	  )
	continue;

      Elf64_Shdr *shdr = elf64_getshdr(scn);
      char *sym_name = elf_strptr(fe->elf,shdr->sh_link, (size_t)sym->st_name);

      if (!sym_name)
	errx(1, "%s : elf_strptr", __func__, elf_errmsg(elf_errno()));

      if (!strcmp(fes->name, sym_name))
	{
	  fes->f = (void (*)())sym->st_value;
	  break;
	}
    }
}

char *_flex_elf_get_buildid(const char *filename)
{
  struct flex_elf fe;
  char *buildid = NULL;

  _flex_elf_open(filename, &fe);
  _flex_elf_map_section(&fe, SHT_NOTE, _flex_mapper_buildid_section, &buildid);
  _flex_elf_close(&fe);

  return buildid;
}

void *_flex_elf_get_sym(const char *filename, const char *fname)
{
  struct flex_elf fe;
  struct flex_elf_sym fes = (struct flex_elf_sym){.name = fname};

  _flex_elf_open(filename, &fe);
  _flex_elf_map_section(&fe, SHT_SYMTAB, _flex_mapper_sym_section, &fes);
  _flex_elf_close(&fe);

  return fes.f;
}
