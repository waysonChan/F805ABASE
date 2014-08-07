#ifndef _ERRORLOG_H
#define _ERRORLOG_H

void	log_msg(const char *, ...);			/* {App misc_source} */
void	log_open(const char *, int, int);
void	log_quit(const char *, ...);
void	log_ret(const char *, ...);
void	log_sys(const char *, ...);

#endif
