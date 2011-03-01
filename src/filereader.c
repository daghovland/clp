/* filereader.c

   Copyright 2008 Free Software Foundation

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA */

/*   Written July and August, 2008 by Dag Hovland, hovlanddag@gmail.com  */



#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

#define BUF_SIZE 16384


char buf[BUF_SIZE];
int fd;
bool file_opened = false;

char * glob_filename = "stdin";

char* line;
size_t cur_buf_size, buf_pos;
size_t line_size;

bool file_finished;

void init_all(){
  file_finished = false;
  buf_pos = 0;
  cur_buf_size = 0;
  line_size = 1024;
  line = malloc_tester(line_size);
  return;
}


void inc_line_size(size_t new_size){
  assert(new_size > line_size);
  line_size = new_size;
  line = realloc_tester(line, line_size);
}

bool init_readfile(char* filename){
  glob_filename = filename;
  if(file_opened)
    close(fd);
  fd = open(filename, O_RDONLY);
  if(fd == -1){
    int error_num = errno;
    if(error_num == EACCES){
      fprintf(stderr, "grep: %s: Ikke tilgang\n", filename);
      return false;
    }
    fprintf(stderr, "file open error on %s: ", filename);
    perror("");
    abort();
  }
  file_opened = true;
  init_all();
  return true;
}

bool init_readstd(){
  fd = STDIN_FILENO;
  init_all();
  return true;
}

void fill_buffer(){
  ssize_t read_ret_val = -1;
  buf_pos = 0;
  assert(!file_finished);
  while(read_ret_val < 0){
    read_ret_val = read(fd, buf, BUF_SIZE);
    if(read_ret_val < 0){
      int error_num = errno;
      if(error_num == EISDIR)
	read_ret_val = 0;
      else if (errno != EINTR){
	fprintf(stderr, "file read error on %s: ", glob_filename);
	perror("");
	abort();
      }
    }
  } 
  cur_buf_size = (size_t) read_ret_val;
  if (cur_buf_size == 0) {
    file_finished = true;
    return;
  }
  return;
}

bool get_file_finished(){
  return file_finished;
}


/**
   line_start is the number of chars that should be skipped before copying text into line
   line_len is set to the number of chars that are read into it,
   such that position of end is line_start + line_len

   line_end is the number of characters that should be allocated at the end of the string
**/
char* nextline(size_t line_start, size_t* line_len, size_t line_end){
  size_t line_pos = line_start; 
  if(file_finished){
    *line_len = 0;
    return line;
  }
  if(buf_pos >= cur_buf_size){
    fill_buffer();
  }

  while(buf[buf_pos] != '\n' && buf[buf_pos] != '\r'){
    while(line_pos < line_size && buf[buf_pos] != '\n' && buf[buf_pos] != '\r' && buf_pos < cur_buf_size){
      line[line_pos++] = buf[buf_pos++];
    }
    if(file_finished)
      break;
    if(buf_pos >= cur_buf_size)
      fill_buffer();
    
    while(line_pos + line_end >= line_size)
      inc_line_size( 2 * line_size );
   
  } 
  if(!file_finished){
    while(!file_finished && buf_pos >= cur_buf_size)
      fill_buffer();
    if(buf[buf_pos] == '\n')
      buf_pos++;
    else if(buf[buf_pos] == '\r'){
      buf_pos++;
      while(!file_finished && buf_pos >= cur_buf_size)
	fill_buffer();
      if(buf_pos < cur_buf_size && buf[buf_pos] == '\n') 
	buf_pos++;
    }
  }
  assert(line_pos - line_start >= 0);
  if(line_pos > line_start || buf_pos != line_pos){
    line[line_pos] = 0;
    *line_len = line_pos - line_start;
  } else {
    *line_len = -1;
    return NULL;
  }
  return line;
}


void end_readfile(){
  close(fd);
  free(line);
}
