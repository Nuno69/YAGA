import idautils
import idc
import idaapi
import re
import os
import ctypes

# Demangler

dbghelp = ctypes.WinDLL("dbghelp")

UnDecorateSymbolName = dbghelp.UnDecorateSymbolName
UnDecorateSymbolName.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint]
UnDecorateSymbolName.restype = ctypes.c_uint

def demangle(name: str) -> str:
    buf = ctypes.create_string_buffer(1024)
    UnDecorateSymbolName(name.encode(), buf, len(buf), 0)
    return buf.value.decode()

# Class signature matching

pattern = re.compile(
    r"""
    ^(?:(?:public|private|protected):\s+)?                                  # optional access specifier
    (?:static\s+)?                                                          # optional 'static'
    (?:virtual\s+)?                                                         # optional 'virtual'
    (?:(?P<return>[\w\s:<>,*&]+?)\s+)?                                      # optional return type
    (?P<callconv>__thiscall|__cdecl|__stdcall|__fastcall|__vectorcall)\s+   # optional calling convention  
    (?:(?P<class>[\w:<>]+)::)?                                              # optional class name (allow nested/templates)
    (?P<method>.+?)\s*                                                      # method name or operator overload
    \((?P<args>.*)\).*$                                                     # argument list
    """,
    re.VERBOSE
)

def parse_signature(signature: str) -> dict:
    """Parse a C++-style function signature into structured components."""
    match = pattern.match(signature)
    if not match:
        return None

    return_type = match.group("return")
    callconv = match.group("callconv")
    class_name = match.group("class") or ""
    method_name = match.group("method")
    args_raw = match.group("args").strip()

    if args_raw == "void" or args_raw == "":
        args = []
    else:
        args = [arg.strip() for arg in args_raw.split(",") if arg.strip()]

    if return_type is None or return_type == "":
        if method_name == class_name:
            return_type = f"{class_name}*"
        else:
            return_type = "void"

    return {
        "return_type": return_type,
        "calling_convention": callconv,
        "class_name": class_name,
        "method_name": method_name,
        "arguments": args,
    }

# Signature file generation

project_path = idaapi.get_input_file_path()
project_name = os.path.splitext(os.path.basename(project_path))[0]

output_file = f"{project_name}.txt"

with open(output_file, "w") as f:
    for func_ea in idautils.Functions():
        raw_name = idc.get_func_name(func_ea)
        if not raw_name:
            continue

        demangled_name = demangle(raw_name)
        #print(f"{hex(func_ea)} {demangled_name}")

        try:
            signature = parse_signature(demangled_name)
            if signature is None:
                print(f"skipped: {demangled_name}")
                continue

            line = (
                f"{hex(func_ea)}"
                f"\t{signature['return_type']}"
                f"\t{signature['calling_convention']}"
                f"\t{signature['class_name']}"
                f"\t{signature['method_name']}"
                f"\t{"\t".join(signature['arguments'])}"
            )

            f.write(line + '\n')

        except Exception as e:
            print(f"Exception occurred: {e}")
            continue

print(f"Exported function list to {output_file}")