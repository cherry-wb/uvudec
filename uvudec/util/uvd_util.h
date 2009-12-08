/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster
JohnDMcMaster@gmail.com
Licensed under terms of the three clause BSD license, see LICENSE for details
*/

#ifndef UV_UTIL_H
#define UV_UTIL_H

#include <stdint.h>
#include <string>
#include "uvd_types.h"
#include "uvd_error.h"

#define UV_TEMP_PREFIX			"/tmp/uvXXXXXX"

/*
Use NULL as a base address
Convert to a pointer
Get the address of a memory location
Since this is a hard coded value, compiler will optimize into a hard coded full address
*/
#define OFFSET_OF(type, field)			((unsigned long) &(((type *) 0)->field))

/*
GNU headers don't cope well with min/max...lame
They don't expose it, but get confused if you try to define it yourself
maybe caused issues with member functions?
Put in namespace maybe and do some template magic?
	#define uvd_min(a, b) uvd_minCore<typeof(a)>(a, b)
	template <typename T> T uvnet::uvd_minCore<T>(T a, T b) ...
*/
#ifndef uvd_min
#define uvd_min(a, b) ({typeof(a) _a = a; typeof(b) _b = b; typeof(a) ret; if( _a > _b ) ret = _a; else ret = _b; ret;})
#endif //uvd_min
#ifndef uvd_max
#define uvd_max(a, b) ({typeof(a) _a = a; typeof(b) _b = b; typeof(a) ret; if( _a < _b ) ret = _a; else ret = _b; ret;})
#endif //uvd_max

std::string uv_basename(const std::string &file);
std::string uv_dirname(const std::string &file);

uv_err_t isRegularFile(const std::string &file);
uv_err_t isDir(const std::string &file);
//If bestEffort is set, willl try to create all dirs needed
uv_err_t createDir(const std::string &file, bool bestEffort);

//Remove comments, maybe more later
uv_err_t uvdPreprocessLine(const std::string &lineIn, std::string &lineOut);
uv_err_t uvdParseLine(const std::string &line_in, std::string &key_in, std::string &value_in);
uv_err_t readFile(const std::string &sFile, std::string &sRet);
uv_err_t writeFile(const std::string &sFile, const std::string &sIn);
uv_err_t writeFile(const std::string &sFile, const char *buff, size_t buffsz);
uv_err_t parseFunc(const std::string &text, std::string &name, std::string &content);

uv_err_t splitConfigLinesVector(const std::vector<std::string> &in, const std::string &delim, std::vector< std::vector<std::string> > &out);

uv_err_t getTempFile(std::string &sFile);
std::string escapeArg(const std::string &sIn);
uv_err_t deleteFile(std::string &sFile);
uv_err_t executeToFile(const std::string &sCommand,
		const std::vector<std::string> &args,
		int &rcProcess,
		const std::string *stdOutIn,
		const std::string *stdErrIn);
uv_err_t executeToText(const std::string &sCommand,
		const std::vector<std::string> &args,
		int &rcProcess,
		std::string *stdOut,
		std::string *stdErr);

std::vector<std::string> split(const std::string &s, char delim, bool ret_blanks = true);
std::vector<std::string> charPtrArrayToVector(char **argv, int argc);

#ifdef __cplusplus
//extern "C"
//{
#endif /* ifdef __cplusplus */

char **uv_split_lines(const char *str, unsigned int *n_ret);
char **uv_split(const char *str, char delim);
char **uv_split_core(const char *str, char delim, unsigned int *n_ret, int ret_blanks);

unsigned int uv_get_num_lines(const char *str);
unsigned int uv_get_num_tokens_core(const char *text, char delim, int ret_blanks);
/* Line numbering starts at 0 */
char *uv_get_line(const char *text, unsigned int lineNumber);

/*
Data returned is malloc'd 
If size is non-NULL, the data size is stored there
If size is NULL, ascii data will be assumed and ret will be null terminated
Error upon file being NULL
If ret is null and size isn't, will report file length
*/
uv_err_t read_file(const char *file, uint8_t **ret, unsigned int *size);
uv_err_t read_filea(const char *file, char **ret);

/*
Instruction capitalization
Make sure its aligned with g_caps option
g_caps = 0: lower case
otherwise: upper case
*/
//const char *inst_caps(const char *inst);
//Change string in place to above policy
//Returns str
char *cap_fixup(char *str);

/* Separate key/value pairs */
uv_err_t uvd_parse_line(const char *line_in, char **key_in, char **value_in);

std::string parseSubstring(const std::string &in, const std::string &seek, const std::string &start, const std::string &end, std::string::size_type *pos = 0);

#ifdef __cplusplus
//}
#endif /* ifdef __cplusplus */

//Get function arguments, parsing parenthesis correctly
uv_err_t getArguments(const std::string &in, std::vector<std::string> &out);

//Used for benchmarking
uint64_t getTimingMicroseconds(void);

#endif /* ifndef UV_UTIL_H */
