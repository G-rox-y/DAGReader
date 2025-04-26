file(READ "${INPUT_VERT}" vert_contents)
file(READ "${INPUT_FRAG}" frag_contents)

foreach(var vert_contents frag_contents)
    string(REPLACE "\"" "\\\"" ${var} "${${var}}") # escape quotation marks
    string(REPLACE "\n" "\\n\"\n\"" ${var} "${${var}}") # convert newline characters
endforeach()

# add to const char variables
file(WRITE "${OUTPUT_FILE}" 
    "const char* vertexShaderSource = \"${vert_contents}\";\n\n"
    "const char* fragmentShaderSource = \"${frag_contents}\";\n"
)