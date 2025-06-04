file(READ "${INPUT_VERT}" vert_contents)
file(READ "${INPUT_TCS}" tcs_contents)
file(READ "${INPUT_TES}" tes_contents)
file(READ "${INPUT_GEOM}" geom_contents)
file(READ "${INPUT_FRAG}" frag_contents)

foreach(var vert_contents tcs_contents tes_contents geom_contents frag_contents)
    string(REPLACE "\"" "\\\"" ${var} "${${var}}") # escape quotation marks
    string(REPLACE "\n" "\\n\"\n\"" ${var} "${${var}}") # convert newline characters
endforeach()

# add to const char variables
file(WRITE "${OUTPUT_FILE}" 
    "const char* vertexShaderSource = \"${vert_contents}\";\n\n"
    "const char* tcsShaderSource = \"${tcs_contents}\";\n\n"
    "const char* tesShaderSource = \"${tes_contents}\";\n\n"
    "const char* geometryShaderSource = \"${geom_contents}\";\n\n"
    "const char* fragmentShaderSource = \"${frag_contents}\";\n"
)