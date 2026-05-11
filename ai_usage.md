# AI Usage Documentation - Phase 1

**Student:** Bordea Sebastian Florin
**AI Tool Used:** Google Gemini

## 1. Generating `parse_condition`

### The Prompt I Gave:
> "I am working on a C project and need to build a filter command. Can you generate a function `int parse_condition(const char *input, char *field, char *op, char *value);` that splits a string formatted as `field:operator:value` (for example `severity:>=:2`) into its three separate string parts?"

### What Was Generated:
The AI generated a function using `sscanf` to slice the string based on the colon `:` character.

>int parse_condition(const char *input, char *field, char *op, char *value) {
if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
return 1;
}
return 0;
}

## 2. Generating `match_condition`

### The Prompt I Gave:
> "I have a C struct that looks like this:
> typedef struct { int severity; char category[100]; } Report;
> I need a function `int match_condition(Report *r, const char *field, const char *op, const char *value);` that returns 1 if the record satisfies the condition and 0 otherwise. It needs to support ==, !=, <, <=, >, >= for the integer severity, and ==, != for the string category."

### What Was Generated:
The AI generated a series of `if-else` statements comparing the strings, and used `atoi()` to convert the `value` string into an integer before comparing it to the `severity` field.

```c
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value); 
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, "<") == 0) return r->severity < val;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    return 0;
}