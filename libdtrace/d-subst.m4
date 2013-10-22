/*
 * Run after the C preprocessor output has been sucked in and transformed
 * into M4 macro definitions, this script provides a macro which generates
 * a D definition with the same integral value as a macro iff that macro
 * is defined.
 */

/*
 * def_constant([[macro]], version) expands to an inline int definition of
 * that macro, expanding to its C preprocessed value, with the same D name.
 *
 * def_constant_renamed(type, D name, [[C macro]], version) expands to an
 * inline definition of the given C macro, with the specified D name and type.
 */

m4_changequote(`[[',`]]')
m4_define([[def_constant]],[[m4_ifdef([[$1]],[[inline int [[$1]] = $1;
#pragma D binding "$2" [[$1]]]])]])
m4_define([[def_constant_renamed]],[[m4_ifdef([[$3]],[[inline [[$1]] [[$2]] = $3;
#pragma D binding "$4" [[$2]]]])]])
m4_divert(0)m4_dnl
