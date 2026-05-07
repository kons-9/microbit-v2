#!/bin/bash
set -e

BASE="$(cd "$(dirname "$0")/../../kernel/mtkernel_3/include" && pwd)"
OUT="$(cd "$(dirname "$0")" && pwd)/include/c"

echo "Source: $BASE"
echo "Output: $OUT"

find "$BASE" -name '*.h' | while read f; do
  rel="${f#$BASE/}"
  outfile="$OUT/$rel"
  mkdir -p "$(dirname "$outfile")"

  guard="__UTKCPP_$(echo "$rel" | tr '/.' '__' | tr '[:lower:]' '[:upper:]')__"
  cat > "$outfile" << EOF
#ifndef $guard
#define $guard
#ifdef __cplusplus
extern "C" {
#endif
#include_next <$rel>
#ifdef __cplusplus
}
#endif
#endif /* $guard */
EOF
done

echo "Done. Generated wrappers:"
find "$OUT" -name '*.h' | wc -l
