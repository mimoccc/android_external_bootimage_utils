#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "program.h"
#include "file.h"
const byte magic_gzip[4]={ 0x1F,0x8B,0x08,0x08 }; //2067208

const byte magic_gzip_no_name[4] =  { 0x1F,0x8B,0x08,0x00 };//529205256


static file_info_enum dirty_magic_compare_offset(const char *filepath, const byte_p magic,size_t read_length,off_t offset){
	byte_p buff =load_file_from_offset(filepath,offset,&read_length);
	if(buff){
		int res =  !memcmp(magic,buff,read_length	);
		free(buff);
		return res;	
	}else return FILE_NO;
}


static file_info_enum dirty_magic_compare(const char *filepath, const byte_p magic,size_t read_length){
	off_t fpt = 0;

	return dirty_magic_compare_offset(filepath ,magic, read_length, fpt);
}
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__) 
int symlink(char *symlink_src,char *filename){ 
	FILE *output_file_fp = fopen(filename, "wb");
    if (output_file_fp != NULL)
    {
		fwrite("LNK:",4,1,output_file_fp);
        fwrite(symlink_src,strlen(symlink_src),1,output_file_fp);
        fclose(output_file_fp);
    }
    return 0;
	
}
ssize_t readlink(const char *path, char *buf, size_t bufsiz) { return 0; }
int vasprintf( char **sptr, const char *fmt, va_list argv ) 
{ 
    int wanted = vsnprintf( *sptr = NULL, 0, fmt, argv ); 
    if( (wanted > 0) && ((*sptr = malloc( 1 + wanted )) != NULL) ) 
        return vsprintf( *sptr, fmt, argv ); 
 
    return wanted; 
} 
#endif
int is_path_directory(char *dname){
	struct stat sb;
	if (stat(dname, &sb) == -1) {
		return 0;
	} 
	return S_ISDIR(sb.st_mode);
}
char *remove_file_extension(char* filename) {
    char *retstr;
    char *lastdot;
    if (filename == NULL)
         return NULL;
    if ((retstr = malloc (strlen (filename) + 1)) == NULL)
        return NULL;
    strcpy (retstr, filename);
    lastdot = strrchr (retstr, '.');
    if (lastdot != NULL)
        *lastdot = '\0';
    return retstr;
}
file_info_enum check_file_exists(char *filename){
	
	struct stat sb;
	if (lstat(filename, &sb) == -1) {
		return FILE_NO;
	}
	
	if(S_ISREG(sb.st_mode) || S_ISBLK(sb.st_mode)|| S_ISLNK(sb.st_mode))
		return FILE_YES;
	else
		return FILE_NO;
		
			/*switch (sb.st_mode & S_IFMT) {
				case S_IFBLK:  log_write("block_device\n");            break;
				case S_IFCHR:  log_write("character_device\n");        break;
				case S_IFDIR:  log_write("directory\n");               break;
				case S_IFIFO:  log_write("fifo_pipe\n");               break;
				case S_IFLNK:  log_write("symlink\n");                 break;
				case S_IFSOCK: log_write("socket\n");                  break;
				case S_IFREG: log_write("regular\n");                  break;
				default:       log_write("unknown?\n");                break;
			}*/
			
	
}
file_info_enum check_directory_exists(char *fname){
	
	struct stat sb;
	if (stat(fname, &sb) == -1) {  
			return FILE_NO;
	}
	if(!S_ISDIR(sb.st_mode)){
		/*if(exitonfailure){
			switch (sb.st_mode & S_IFMT) {
				case S_IFBLK:  printf("block device\n");            break;
				case S_IFCHR:  printf("character device\n");        break;
				case S_IFREG:  printf("regular file\n");            break;
				case S_IFIFO:  printf("FIFO/pipe\n");               break;
				case S_IFLNK:  printf("symlink\n");                 break;
				case S_IFSOCK: printf("socket\n");                  break;
				default:       printf("unknown?\n");                break;
			}        
			exit(EXIT_FAILURE);
		
		}*/
		return FILE_NO;
	}else
		return FILE_YES ;	//S_ISDIR(sb.st_mode);
}
file_info_enum is_cpio_file(const char *filepath)
{
	return dirty_magic_compare(filepath,(const byte_p)magic_cpio_ascii,sizeof(magic_cpio_ascii));		
}
file_info_enum is_android_boot_image_file(const char *filepath){
	unsigned int filesize=0;	file_info_enum res = FILE_NO;
	byte_p data = load_file(filepath,&filesize);
	if(data){
		if(find_in_memory(data,filesize,BOOT_MAGIC,BOOT_MAGIC_SIZE))
			res=FILE_YES;
		free(data);
	}
	return res;
	
}
file_info_enum is_gzip_file(const char *filepath)
{
	file_info_enum res = dirty_magic_compare(filepath,(const byte_p)magic_gzip,4);
	if(!res) res = dirty_magic_compare(filepath,(const byte_p)magic_gzip_no_name,4);
	return res;			
}
// Crude ascii checker
file_info_enum is_windows_text(const byte_p stream, const size_t size){
	if(is_ascii_text(stream,size))
		if(strstr((const char*)stream,WINDOWS_EOL))
			return FILE_YES;
	return FILE_NO;
}
file_info_enum is_ascii_text(const byte_p stream, const size_t size){
	return (file_info_enum)!memchr(stream,(int)NULL,size);
}
byte_p load_file_from_offset(const char *filepath,off_t offset,size_t *file_size)
{
	
    unsigned char *data;  size_t sz; int fd; 
    size_t size_store=(*file_size);
    
    data = 0;
    FILE *fp = fopen(filepath, "rb");
    fd = fileno(fp);
    if(fd < 0) return 0;
    sz = lseek(fd, 0, SEEK_END);
    if((int)sz < 0) goto oops;
	rewind(fp);
	
    if(lseek(fd, offset, SEEK_SET) != offset) goto oops;
	if(size_store > 0 ){
		sz=read(fd, data,10);
		 //!= size_store) goto oops; 
		close(fd);
		return data;
	}
	
	// check if we have set a file_size 
	sz = (*file_size)>0 ? (*file_size) : sz-offset;
	
    data = (unsigned char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != (int)sz) goto oops; 
    close(fd);

    if(file_size) *file_size = sz;
    return data;

oops:
    fclose(fp);
    if(data != 0) free(data);
    return 0;
}

int write_to_file_mode(byte_p data_in, size_t output_size,char * output_filename, mode_t mode){
	write_to_file(data_in,output_size,output_filename);
	chmod(output_filename,mode);
	return 0;
}
int write_to_file(byte_p data_in, size_t output_size,char * output_filename){
	// Validate input
	
	FILE *output_file_fp;
	//fprintf(stderr,"Fullpath:%s\n",output_filename);
	if((output_file_fp = fopen(output_filename,"wb"))== NULL){
		printf("Cannot open %s file!\n",output_filename);
		goto oops;
	}
	if(output_file_fp)
	{
		fwrite(data_in,output_size,1,output_file_fp);
		fclose(output_file_fp);

	}
oops:
	return 0;
}
int dump_file(FILE* boot_image_fp,char * output_filename,unsigned offset,unsigned size){
	
	// assign some memory for the read
	void *output_buffer = malloc(size);

	// goto the offset of the file we are looking
	fseek(boot_image_fp, offset, SEEK_SET);
	// smash it into the output buffer
	fread(output_buffer,size,1,boot_image_fp);
	
	FILE *output_file_fp;
	//fprintf(stderr,"Fullpath:%s\n",fullpath);
	if((output_file_fp = fopen(output_filename,"wb"))== NULL){
		printf("Cannot open %s file!\n",output_filename);
		goto oops;
	}
	if(output_file_fp)
	{
		fwrite(output_buffer,size,1,output_file_fp);
		fclose(output_file_fp);
	}
oops:
	if(!output_buffer)
		free(output_buffer);
	return 0;
}
/* 
 * if successful load_file will return a pointer to the start of the data
 * The memory allocated by this function must be freed by the  caller 
 */
byte_p load_file(const char *filename, size_t *file_size)
{
	return load_file_from_offset(filename , (off_t)0 , file_size) ;
}
byte_p find_string_in_memory(const byte_p haystack, size_t haystack_len, const char * needle){
	
	size_t begin=0;size_t len = strlen(needle);
	fprintf(stderr,"HS:%p HL:%ud N:%s\n",haystack,	haystack_len,needle);
	for(begin=0 ; begin< haystack_len; begin++){
		if(haystack[begin]==(byte)needle[0]){
			 if(!strncmp(needle,(char const*)haystack+begin,len+1)) return (byte_p)haystack+begin;
		}
	}
	return NULL;
}
byte_p find_in_memory(const byte_p haystack, size_t haystack_len, const void * needle,  size_t needle_len)
{
  const char *begin;
  const char *const last_possible = (const char *) haystack + haystack_len - needle_len;

  if (needle_len == 0)
    /* The first occurrence of the empty string is deemed to occur at the beginning of the string.  */
    return (byte_p) haystack;

  /* Sanity check, otherwise the loop might search through the whole memory.  */
  if (__builtin_expect (haystack_len < needle_len, 0))
    return NULL;

  for (begin = (const char *) haystack; begin <= last_possible; ++begin)
    if (begin[0] == ((const char *) needle)[0] &&
	!memcmp ((const void *) &begin[1],
		 (const void *) ((const char *) needle + 1),
		 needle_len - 1))
      return (void *) begin;

  return NULL;
}

size_t dos_to_unix(byte_p output_buffer, const byte_p input_buffer)
{
    byte_p p = input_buffer;
    byte_p q = output_buffer;
    size_t times =0;
    while (*p) {
        if (p[0] == '\r' && p[1] == '\n') {
            // dos
            *q = '\n';
            p += 2;
            q += 1;
            times+=1;
        }
        else if (p[0] == '\r') {
            // old mac
            *q = '\n';
            p += 1;
            q += 1;
        }
        else {
            *q = *p;
            p += 1;
            q += 1;
        }
    }
    *q = '\0';
    return times;
}

size_t  unix_to_dos(byte_p output_buffer, const byte_p input_buffer)
{
    byte_p p = input_buffer;
    byte_p q = output_buffer;
    size_t times=0;
    while (*p) {
        if (*p == '\n') {
            q[0] = '\r';
            q[1] = '\n';
            q += 2;
            p += 1;
            times+=1; 
        } else {
            *q = *p;
            p += 1;
            q += 1;
        }
    }
    *q = '\0';
    return times;
}
void mkdir_and_parents(const char *path,mode_t mode)
{
        char opath[256];
        char *p;
        size_t len;

        strncpy(opath, path, sizeof(opath));
        len = strlen(opath);
        if(opath[len - 1] == '/')
                opath[len - 1] = '\0';
        for(p = opath; *p; p++)
                if(*p == '/') {
                        *p = '\0';
                        if(access(opath, F_OK))
                                mkdir(opath, mode);
                        *p = '/';
                }
        if(access(opath, F_OK))         /* if path is not terminated with / */
                mkdir(opath, mode);
}
file_info_enum confirm_file_replace(const char *source_filename,const char *target_filename){
	fprintf(stderr,"file %s already exists\nReplace with %s? (Yes/No/All) [Y]",source_filename,target_filename);
	char answer = getchar();
	
	if(answer=='a' || answer=='A' ) return FILE_ALL;
	if(answer=='Y' || answer=='y' || answer==10) return FILE_YES; 
	if(answer=='N' || answer=='n') return FILE_NO;
	return FILE_NO;
}
