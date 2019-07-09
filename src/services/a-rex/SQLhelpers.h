#ifndef __AREX_SQL_COMMON_HELPERS_H__
#define __AREX_SQL_COMMON_HELPERS_H__

#include <string>

#include <arc/StringConv.h>
#include <arc/DateTime.h>

namespace ARex {

static const std::string sql_special_chars("'#\r\n\b\0",6);
static const char sql_escape_char('%');
static const Arc::escape_type sql_escape_type(Arc::escape_hex);

// Returns SQL-escaped string representation of argumnet
inline static std::string sql_escape(const std::string& str) {
    return Arc::escape_chars(str, sql_special_chars, sql_escape_char, false, sql_escape_type);
}

inline static std::string sql_escape(int num) {
    return Arc::tostring(num);
}

inline static std::string sql_escape(const Arc::Time& val) {
    if(val.GetTime() == -1) return "";
    return Arc::escape_chars((std::string)val, sql_special_chars, sql_escape_char, false, sql_escape_type);
}

// Unescape SQLite returned values
inline static std::string sql_unescape(const std::string& str) {
    return Arc::unescape_chars(str, sql_escape_char,sql_escape_type);
}

inline static void sql_unescape(const std::string& str, int& val) {
    (void)Arc::stringto(Arc::unescape_chars(str, sql_escape_char,sql_escape_type), val);
}

inline static void sql_unescape(const std::string& str, Arc::Time& val) {
    if(str.empty()) { val = Arc::Time(); return; }
    val = Arc::Time(Arc::unescape_chars(str, sql_escape_char,sql_escape_type));
}

} // namespace ARex

#endif // __AREX_SQL_COMMON_HELPERS_H__
