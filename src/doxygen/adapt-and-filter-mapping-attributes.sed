#!/bin/sed -f

# Copy mapdef ID to buffer
/\\mapdef / {
  # Copy current line to buffer
  h
  # Remove every thing but mapdef ID.
  s/.*\\mapdef \([^[:space:]]\+\)[[:space:]]\+.*/\1/
  # Swap buffer with pattern space.
  x
}

# Remove \mapdef attribute plus associated description. End at first empty line,
# line with asterisks (*) or line with asterisks followed by slash (/) modulo
# spaces.
/\\mapdef /,/^[[:space:]]*\**\/\?[[:space:]]*$/ {
  /^[[:space:]]*\**\/\?[[:space:]]*$/ ! d
}

# Replace mapdefattr command with link to attribute mapping.
/\\mapdefattr/ {
  # Append buffer (prefixed with new line) to pattern space. This should be the
  # mapdef ID copied above. Thus the assumption is that the mapdef command must
  # come before the mapdefattr command.
  G
  
  # Replace \mapdefattr line with a link pointing to mapping of specific
  # attribute.
  #                            mapdefattr name                             mapdef ID
  s/\\mapdefattr[[:space:]]\+\([^[:space:]]\+\)[[:space:]]\+[^[:space:]]\+\n\(.*\)$/\2.html#attr_\1/
  s/[^[:space:]]\+$/<a href=\"&\">Attribute mapping specific to this field\/value.<\/a>/
  # :: should be transformed to _ in URLs.
  s/::/_/g
}

