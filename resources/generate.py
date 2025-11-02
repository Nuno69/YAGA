import idautils
import idc
import idaapi
import ida_name
import ida_funcs
import re
import os

project_path = idaapi.get_input_file_path()
project_name = os.path.splitext(os.path.basename(project_path))[0]

output_file = f"{project_name}.txt"

# Regex to match full function signature
signature_regex = re.compile(
    r'(.+)(__thiscall)\s+(?:(\w+)::)?(~?\w+)\((.*)\)'
)

with open(output_file, "w") as f:
    for func_ea in idautils.Functions():
        raw_name = idc.get_func_name(func_ea)
        if not raw_name:
            continue

        clean_name = ida_name.demangle_name(raw_name, ida_name.MNG_NODEFINIT)
        if not clean_name:
            continue

        full_sig = idaapi.print_type(func_ea, idaapi.PRTYPE_1LINE | idaapi.PRTYPE_CPP)
        if not full_sig:
            continue

        func = idaapi.get_func(func_ea)
        if func and (func.flags & ida_funcs.FUNC_THUNK):
            continue

        try:
            match = signature_regex.match(full_sig)
            if not match:
                continue

            type_part = match.group(1).strip()
            cc_part = match.group(2)
            class_part = match.group(3) or ""
            name_part = match.group(4)
            args_str = match.group(5)

            # Split arguments manually
            args_part = [
                arg.strip()
                    .replace("struct ", "")
                    .replace(" *", "*")
                for arg in args_str.split(',')
                if arg.strip() and '__hidden this' not in arg
            ]

            line = f"{hex(func_ea)}\t{type_part}\t{cc_part}\t{class_part}\t{name_part}"
            if args_part:
                line += '\t' + '\t'.join(args_part)
            f.write(line + '\n')

        except Exception:
            continue

print(f"Exported function list to {output_file}")