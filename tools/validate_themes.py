import json
from os import wait
import sys

if len(sys.argv) < 2:
    print(f"USAGE: python3 {sys.argv[0]} [ppp file]");
    exit(1);

file_path = sys.argv[1];

file = open(file_path, "r");
content = file.read();

theme_validation_str = "const THEMES = {";
idx = content.find(theme_validation_str);

if (idx == -1):
    print(f"could not find theme declaration: `{theme_validation_str}`");
    exit(1);

opening_brackets = 1;
end_idx = -1;

for i in range (idx + len(theme_validation_str), len(content)):
    if (content[i] == '{'):
        opening_brackets += 1;

    if (content[i] == '}'):
        opening_brackets -= 1;

    if (opening_brackets == 0):
        end_idx = i;
        break;

if (end_idx == -1):
    print("could not find end of theme declaration");
    exit(1);

needed_substr = content[idx + len(theme_validation_str) - 1:end_idx + 1];

theme_json = json.loads(needed_substr);

all_keys = set()

for theme in theme_json.values():
    all_keys.update(theme.keys())

has_atleaast_1_missing = False;
for theme_name, theme in theme_json.items():
    missing = all_keys - theme.keys()
    extra = theme.keys() - all_keys

    if missing or extra:
        print(f"\nTheme: {theme_name}")
        if missing:
            print("  Missing keys:", missing)
            has_atleaast_1_missing = True;
        if extra:
            print("  Extra keys:", extra)

if (has_atleaast_1_missing == True):
    exit(1);
