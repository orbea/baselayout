/*
 * misc.c
 *
 * Miscellaneous macro's and functions.
 *
 * Copyright (C) 2004,2005 Martin Schlemmer <azarah@nosferatu.za.org>
 *
 *
 *      This program is free software; you can redistribute it and/or modify it
 *      under the terms of the GNU General Public License as published by the
 *      Free Software Foundation version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Header$
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "rcscripts.h"

char *
memrepchr (char **str, char old, char new, size_t size)
{
  char *str_p;

  if (!check_arg_strv (str))
    return NULL;

  str_p = memchr (*str, old, size);

  while (NULL != str_p)
    {
      str_p[0] = new;
      str_p = memchr (&str_p[1], old, size - (str_p - *str) - 1);
    }

  return *str;
}

char *
strcatpaths (const char *pathname1, const char *pathname2)
{
  char *new_path = NULL;
  int lenght;

  if ((!check_arg_str (pathname1)) || (!check_arg_str (pathname2)))
    return 0;

  /* Lenght of pathname1 + lenght of pathname2 + '/' if needed */
  lenght = strlen (pathname1) + strlen (pathname2) + 2;
  /* lenght + '\0' */
  new_path = xmalloc (lenght);
  if (NULL == new_path)
    return NULL;

  snprintf (new_path, lenght, "%s%s%s", pathname1,
	    (new_path[strlen (new_path) - 1] != '/') ? "/" : "",
	    pathname2);

  return new_path;
}

char *
strndup (const char *str, size_t size)
{
  char *new_str = NULL;
  size_t len;

  /* We cannot check if its a valid string here, as it might
   * not be '\0' terminated ... */
  if (!check_arg_ptr (str))
    return NULL;

  /* Check lenght of str without breaching the size limit */
  for (len = 0; (len < size) && ('\0' != str[len]); len++);

  new_str = xmalloc (len + 1);
  if (NULL == new_str)
    return NULL;

  /* Make sure our string is NULL terminated */
  new_str[len] = '\0';

  return (char *) memcpy (new_str, str, len);
}

char *
gbasename (const char *path)
{
  char *new_path = NULL;

  if (!check_arg_str (path))
    return NULL;

  /* Copied from glibc */
  new_path = strrchr (path, '/');
  return new_path ? new_path + 1 : (char *) path;
}


int
exists (const char *pathname)
{
  struct stat buf;
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  retval = lstat (pathname, &buf);
  if (-1 != retval)
    return 1;

  /* Clear errno, as we do not want debugging to trigger */
  errno = 0;

  return 0;
}

int
is_file (const char *pathname, int follow_link)
{
  struct stat buf;
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  retval = follow_link ? stat (pathname, &buf) : lstat (pathname, &buf);
  if ((-1 != retval) && (S_ISREG (buf.st_mode)))
    return 1;

  /* Clear errno, as we do not want debugging to trigger */
  errno = 0;

  return 0;
}

int
is_link (const char *pathname)
{
  struct stat buf;
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  retval = lstat (pathname, &buf);
  if ((-1 != retval) && (S_ISLNK (buf.st_mode)))
    return 1;

  /* Clear errno, as we do not want debugging to trigger */
  errno = 0;

  return 0;
}

int
is_dir (const char *pathname, int follow_link)
{
  struct stat buf;
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  retval = follow_link ? stat (pathname, &buf) : lstat (pathname, &buf);
  if ((-1 != retval) && (S_ISDIR (buf.st_mode)))
    return 1;

  /* Clear errno, as we do not want debugging to trigger */
  errno = 0;

  return 0;
}

time_t
get_mtime (const char *pathname, int follow_link)
{
  struct stat buf;
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  retval = follow_link ? stat (pathname, &buf) : lstat (pathname, &buf);
  if (-1 != retval)
    return buf.st_mtime;

  /* Clear errno, as we do not want debugging to trigger */
  errno = 0;

  return 0;
}

#if !defined(HAVE_REMOVE)
int
remove (const char *pathname)
{
  int retval;

  if (!check_arg_str (pathname))
    return -1;

  if (1 == is_dir (pathname, 0))
    retval = rmdir (pathname);
  else
    retval = unlink (pathname);

  return retval;
}
#endif

int
mktree (const char *pathname, mode_t mode)
{
  char *temp_name = NULL;
  char *temp_token = NULL;
  char *token_p;
  char *token;
  int retval;
  int lenght;

  if (!check_arg_str (pathname))
    return -1;

  /* Lenght of 'pathname' + extra for "./" if needed */
  lenght = strlen (pathname) + 2;
  /* lenght + '\0' */
  temp_name = xmalloc (lenght + 1);
  if (NULL == temp_name)
    return -1;

  temp_token = xstrndup (pathname, strlen (pathname));
  if (NULL == temp_token)
    goto error;

  token_p = temp_token;

  if (pathname[0] == '/')
    temp_name[0] = '\0';
  else
    /* If not an absolute path, make it local */
    strncpy (temp_name, ".", lenght);

  token = strsep (&token_p, "/");
  /* First token might be "", but that is OK as it will be when the
   * pathname starts with '/' */
  while (NULL != token)
    {
      strncat (temp_name, "/", lenght - strlen (temp_name));
      strncat (temp_name, token, lenght - strlen (temp_name));

      /* If it does not exist, create the dir.  If it does exit,
       * but is not a directory, we will catch it below. */
      if (1 != exists (temp_name))
	{
	  retval = mkdir (temp_name, mode);
	  if (-1 == retval)
	    {
	      DBG_MSG ("Failed to create directory!\n");
	      goto error;
	    }
	  /* Not a directory or symlink pointing to a directory */
	}
      else if (1 != is_dir (temp_name, 1))
	{
	  errno = ENOTDIR;
	  DBG_MSG ("Component in pathname is not a directory!\n");
	  goto error;
	}

      do
	{
	  token = strsep (&token_p, "/");
	  /* The first "" was Ok, but rather skip double '/' after that */
	}
      while ((NULL != token) && (0 == strlen (token)));
    }

  free (temp_name);
  free (temp_token);

  return 0;

error:
  free (temp_name);
  free (temp_token);

  return -1;
}

int
rmtree (const char *pathname)
{
  char **dirlist = NULL;
  int i = 0;

  if (!check_arg_str (pathname))
    return -1;

  if (1 != exists (pathname))
    {
      errno = ENOENT;
      DBG_MSG ("'%s' does not exists!\n", pathname);
      return -1;
    }

  dirlist = ls_dir (pathname, 1);
  if ((NULL == dirlist) && (0 != errno))
    {
      /* Do not error out - caller should decide itself if it
       * it is an issue */
      DBG_MSG ("Could not get listing for '%s'!\n", pathname);
      return -1;
    }

  while ((NULL != dirlist) && (NULL != dirlist[i]))
    {
      /* If it is a directory, call rmtree() again with
       * it as argument */
      if (1 == is_dir (dirlist[i], 0))
	{
	  if (-1 == rmtree (dirlist[i]))
	    {
	      DBG_MSG ("Failed to delete sub directory!\n");
	      goto error;
	    }
	}

      /* Now actually remove it.  Note that if it was a directory,
       * it should already be removed by above rmtree() call */
      if ((1 == exists (dirlist[i]) && (-1 == remove (dirlist[i]))))
	{
	  DBG_MSG ("Failed to remove '%s'!\n", dirlist[i]);
	  goto error;
	}
      i++;
    }

  str_list_free (dirlist);

  /* Now remove the parent */
  if (-1 == remove (pathname))
    {
      DBG_MSG ("Failed to remove '%s'!\n", pathname);
      goto error;
    }

  return 0;
error:
  str_list_free (dirlist);

  return -1;
}

char **
ls_dir (const char *pathname, int hidden)
{
  DIR *dp;
  struct dirent *dir_entry;
  char **dirlist = NULL;

  if (!check_arg_str (pathname))
    return NULL;

  dp = opendir (pathname);
  if (NULL == dp)
    {
      DBG_MSG ("Failed to call opendir()!\n");
      /* errno will be set by opendir() */
      goto error;
    }

  do
    {
      /* Clear errno to distinguish between EOF and error */
      errno = 0;
      dir_entry = readdir (dp);
      /* Only an error if 'errno' != 0, else EOF */
      if ((NULL == dir_entry) && (0 != errno))
	{
	  DBG_MSG ("Failed to call readdir()!\n");
	  goto error;
	}
      if ((NULL != dir_entry)
	  /* Should we display hidden files? */
	  && (hidden ? 1 : dir_entry->d_name[0] != '.'))
	{
	  char *d_name = dir_entry->d_name;
	  char *str_ptr;

	  /* Do not list current or parent entries */
	  if ((0 == strcmp (d_name, ".")) || (0 == strcmp (d_name, "..")))
	    continue;

	  str_ptr = strcatpaths (pathname, d_name);
	  if (NULL == str_ptr)
	    {
	      DBG_MSG ("Failed to allocate buffer!\n");
	      goto error;
	    }

	  str_list_add_item (dirlist, str_ptr, error);
	}
    }
  while (NULL != dir_entry);

  if (!check_strv (dirlist))
    {
      if (NULL != dirlist)
	str_list_free (dirlist);

      DBG_MSG ("Directory is empty.\n");
    }

  closedir (dp);

  return dirlist;

error:
  /* Free dirlist on error */
  str_list_free (dirlist);

  if (NULL != dp)
    {
      save_errno ();
      closedir (dp);
      /* closedir() might have changed it */
      restore_errno ();
    }

  return NULL;
}

/* This handles simple 'entry="bar"' type variables.  If it is more complex
 * ('entry="$(pwd)"' or such), it will obviously not work, but current behaviour
 * should be fine for the type of variables we want. */
char *
get_cnf_entry (const char *pathname, const char *entry)
{
  dyn_buf_t *dynbuf = NULL;
  char *buf = NULL;
  char *str_ptr;
  char *value = NULL;
  char *token;


  if ((!check_arg_str (pathname)) || (!check_arg_str (entry)))
    return NULL;

  /* If it is not a file or symlink pointing to a file, bail */
  if (1 != is_file (pathname, 1))
    {
      errno = ENOENT;
      DBG_MSG ("Given pathname is not a file or do not exist!\n");
      return NULL;
    }

  dynbuf = new_dyn_buf_mmap_file (pathname);
  if (NULL == dynbuf)
    {
      DBG_MSG ("Could not open config file for reading!\n");
      return NULL;
    }

  while (NULL != (buf = read_line_dyn_buf (dynbuf)))
    {
      str_ptr = buf;

      /* Strip leading spaces/tabs */
      while ((str_ptr[0] == ' ') || (str_ptr[0] == '\t'))
	str_ptr++;

      /* Get entry and value */
      token = strsep (&str_ptr, "=");
      /* Bogus entry or value */
      if (NULL == token)
	goto _continue;

      /* Make sure we have a string that is larger than 'entry', and
       * the first part equals 'entry' */
      if ((strlen (token) > 0) && (0 == strcmp (entry, token)))
	{
	  do
	    {
	      /* Bash variables are usually quoted */
	      token = strsep (&str_ptr, "\"\'");
	      /* If quoted, the first match will be "" */
	    }
	  while ((NULL != token) && (0 == strlen (token)));

	  /* We have a 'entry='.  We respect bash rules, so NULL
	   * value for now (if not already) */
	  if (NULL == token)
	    {
	      /* We might have 'entry=' and later 'entry="bar"',
	       * so just continue for now ... we will handle
	       * it below when 'value == NULL' */
	      if (NULL != value)
		{
		  free (value);
		  value = NULL;
		}
	      goto _continue;
	    }

	  /* If we have already allocated 'value', free it */
	  if (NULL != value)
	    free (value);

	  value = xstrndup (token, strlen (token));
	  if (NULL == value)
	    {
	      free_dyn_buf (dynbuf);
	      free (buf);

	      return NULL;
	    }

	  /* We do not break, as there might be more than one entry
	   * defined, and as bash uses the last, so should we */
	  /* break; */
	}

_continue:
      free (buf);
    }

  /* read_line_dyn_buf() returned NULL with errno set */
  if ((NULL == buf) && (0 != errno))
    {
      DBG_MSG ("Failed to read line from dynamic buffer!\n");
      free_dyn_buf (dynbuf);
      if (NULL != value)
	free (value);

      return NULL;
    }


  if (NULL == value)
    DBG_MSG ("Failed to get value for config entry '%s'!\n", entry);

  free_dyn_buf (dynbuf);

  return value;
}

char **
get_list_file (char **list, char *filename)
{
  char *buf = NULL;
  char *tmp_buf = NULL;
  char *tmp_p = NULL;
  char *token = NULL;
  size_t lenght = 0;
  int count = 0;
  int current = 0;

  if (-1 == file_map (filename, &buf, &lenght))
    return NULL;

  while (current < lenght)
    {
      count = buf_get_line (buf, lenght, current);

      tmp_buf = xstrndup (&buf[current], count);
      if (NULL == tmp_buf)
	goto error;

      tmp_p = tmp_buf;

      /* Strip leading spaces/tabs */
      while ((tmp_p[0] == ' ') || (tmp_p[0] == '\t'))
	tmp_p++;

      /* Get entry - we do not want comments, and only the first word
       * on a line is valid */
      token = strsep (&tmp_p, "# \t");
      if (check_str (token))
	{
	  tmp_p = xstrndup (token, strlen (token));
	  if (NULL == tmp_p)
	    goto error;

	  str_list_add_item (list, tmp_p, error);
	}

      current += count + 1;
      free (tmp_buf);
      /* Set to NULL in case we error out above and have
       * to free below */
      tmp_buf = NULL;
    }


  file_unmap (buf, lenght);

  return list;

error:
  if (NULL != tmp_buf)
    free (tmp_buf);
  file_unmap (buf, lenght);
  str_list_free (list);

  return NULL;
}


/*
 * Below three functions (file_map, file_unmap and buf_get_line) are
 * from udev-050 (udev_utils.c).
 * (Some are slightly modified, please check udev for originals.)
 *
 * Copyright (C) 2004 Kay Sievers <kay@vrfy.org>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 * 
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

int
file_map (const char *filename, char **buf, size_t * bufsize)
{
  struct stat stats;
  int fd;

  fd = open (filename, O_RDONLY);
  if (fd < 0)
    {
      DBG_MSG ("Failed to open file!\n");
      return -1;
    }

  if (fstat (fd, &stats) < 0)
    {
      DBG_MSG ("Failed to stat file!\n");

      save_errno ();
      close (fd);
      restore_errno ();

      return -1;
    }

  if (0 == stats.st_size)
    {
      errno = EINVAL;
      DBG_MSG ("Failed to mmap file with 0 size!\n");

      save_errno ();
      close (fd);
      restore_errno ();

      return -1;
    }

  *buf = mmap (NULL, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (*buf == MAP_FAILED)
    {
      DBG_MSG ("Failed to mmap file!\n");

      save_errno ();
      close (fd);
      restore_errno ();

      return -1;
    }
  *bufsize = stats.st_size;

  close (fd);

  return 0;
}

void
file_unmap (char *buf, size_t bufsize)
{
  munmap (buf, bufsize);
}

size_t
buf_get_line (char *buf, size_t buflen, size_t cur)
{
  size_t count = 0;

  for (count = cur; count < buflen && buf[count] != '\n'; count++);

  return count - cur;
}
