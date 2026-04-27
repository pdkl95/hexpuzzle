/*
 * adapted from: https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
 */

#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

//Regular text
#define ANSI_COLOR_BLACK   "\e[0;30m"
#define ANSI_COLOR_RED     "\e[0;31m"
#define ANSI_COLOR_GREEN   "\e[0;32m"
#define ANSI_COLOR_YELLOW  "\e[0;33m"
#define ANSI_COLOR_BLUE    "\e[0;34m"
#define ANSI_COLOR_MAGENTA "\e[0;35m"
#define ANSI_COLOR_CYAN    "\e[0;36m"
#define ANSI_COLOR_WHITE   "\e[0;37m"

//Regular bold text
#define ANSI_COLOR_BOLD_BLACK   "\e[1;30m"
#define ANSI_COLOR_BOLD_RED     "\e[1;31m"
#define ANSI_COLOR_BOLD_GREEN   "\e[1;32m"
#define ANSI_COLOR_BOLD_YELLOW  "\e[1;33m"
#define ANSI_COLOR_BOLD_BLUE    "\e[1;34m"
#define ANSI_COLOR_BOLD_MAGENTA "\e[1;35m"
#define ANSI_COLOR_BOLD_CYAN    "\e[1;36m"
#define ANSI_COLOR_BOLD_WHITE   "\e[1;37m"

//Regular underline text
#define ANSI_COLOR_UNDERLINE_BLACK   "\e[4;30m"
#define ANSI_COLOR_UNDERLINE_RED     "\e[4;31m"
#define ANSI_COLOR_UNDERLINE_GREEN   "\e[4;32m"
#define ANSI_COLOR_UNDERLINE_YELLOW  "\e[4;33m"
#define ANSI_COLOR_UNDERLINE_BLUE    "\e[4;34m"
#define ANSI_COLOR_UNDERLINE_MAGENTA "\e[4;35m"
#define ANSI_COLOR_UNDERLINE_CYAN    "\e[4;36m"
#define ANSI_COLOR_UNDERLINE_WHITE   "\e[4;37m"

//Regular background
#define ANSI_COLOR_BLACK_BG   "\e[40m"
#define ANSI_COLOR_RED_BG     "\e[41m"
#define ANSI_COLOR_GREEN_BG   "\e[42m"
#define ANSI_COLOR_YELLOW_BG  "\e[43m"
#define ANSI_COLOR_BLUE_BG    "\e[44m"
#define ANSI_COLOR_MAGENTA_BG "\e[45m"
#define ANSI_COLOR_CYAN_BG    "\e[46m"
#define ANSI_COLOR_WHITE_BG   "\e[47m"

//High intensty background 
#define ANSI_COLOR_BLACK_HIGH_BG   "\e[100m"
#define ANSI_COLOR_RED_HIGH_BG     "\e[101m"
#define ANSI_COLOR_GREEN_HIGH_BG   "\e[102m"
#define ANSI_COLOR_YELLOW_HIGH_BG  "\e[103m"
#define ANSI_COLOR_BLUE_HIGH_BG    "\e[104m"
#define ANSI_COLOR_MAGENTA_HIGH_BG "\e[105m"
#define ANSI_COLOR_CYAN_HIGH_BG    "\e[106m"
#define ANSI_COLOR_WHITE_HIGH_BG   "\e[107m"

//High intensty text
#define ANSI_COLOR_HIGH_BLACK   "\e[0;90m"
#define ANSI_COLOR_HIGH_RED     "\e[0;91m"
#define ANSI_COLOR_HIGH_GREEN   "\e[0;92m"
#define ANSI_COLOR_HIGH_YELLOW  "\e[0;93m"
#define ANSI_COLOR_HIGH_BLUE    "\e[0;94m"
#define ANSI_COLOR_HIGH_MAGENTA "\e[0;95m"
#define ANSI_COLOR_HIGH_CYAN    "\e[0;96m"
#define ANSI_COLOR_HIGH_WHITE   "\e[0;97m"

//Bold high intensity text
#define ANSI_COLOR_BOLD_HIGH_BLACK   "\e[1;90m"
#define ANSI_COLOR_BOLD_HIGH_RED     "\e[1;91m"
#define ANSI_COLOR_BOLD_HIGH_GREEN   "\e[1;92m"
#define ANSI_COLOR_BOLD_HIGH_YELLOW  "\e[1;93m"
#define ANSI_COLOR_BOLD_HIGH_BLUE    "\e[1;94m"
#define ANSI_COLOR_BOLD_HIGH_MAGENTA "\e[1;95m"
#define ANSI_COLOR_BOLD_HIGH_CYAN    "\e[1;96m"
#define ANSI_COLOR_BOLD_HIGH_WHITE   "\e[1;97m"

//Underline high intensity text
#define ANSI_COLOR_UNDERLINE_HIGH_BLACK   "\e[4;90m"
#define ANSI_COLOR_UNDERLINE_HIGH_RED     "\e[4;91m"
#define ANSI_COLOR_UNDERLINE_HIGH_GREEN   "\e[4;92m"
#define ANSI_COLOR_UNDERLINE_HIGH_YELLOW  "\e[4;93m"
#define ANSI_COLOR_UNDERLINE_HIGH_BLUE    "\e[4;94m"
#define ANSI_COLOR_UNDERLINE_HIGH_MAGENTA "\e[4;95m"
#define ANSI_COLOR_UNDERLINE_HIGH_CYAN    "\e[4;96m"
#define ANSI_COLOR_UNDERLINE_HIGH_WHITE   "\e[4;97m"

//Reset
#define ANSI_COLOR_COLOR_RESET "\e[0m"

#endif /*ANSI_COLORS_H*/
