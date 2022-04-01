import re, os, sys, tempfile

BASE_FILE = tempfile.mkdtemp() + "/code"
U32_1 = (2**32) - 1
ASSEMBLER = (
    f"arm-none-eabi-as -mcpu=arm1176jzf-s -march=armv6zk {BASE_FILE}.s -o {BASE_FILE}.o"
)
OBJDUMP = f"arm-none-eabi-objcopy {BASE_FILE}.o -O binary {BASE_FILE}.bin"

REGS = [f"r{i}" for i in range(0, 16)]
IMMS = ["#0", "#1", "#2", "#3", "#15", "#31", "#63", "#127", "#255"]
BIG = [f"{i}" for i in range(1, 2**24, 90001)]
LABS = ["before", "after"]

VAR_FORMAT = re.compile(r"\(\w+:\w+\)")

inst = sys.argv[1]
vars = VAR_FORMAT.findall(inst)


def encoding(instructions):
    with open(BASE_FILE + ".s", "w") as outfile:
        outfile.write(instructions + "\n")
    os.system(ASSEMBLER)
    os.system(OBJDUMP)
    with open(BASE_FILE + ".bin", "rb") as infile:
        return int.from_bytes(infile.read(), "little")


def var_options(var):
    return {
        "reg": REGS,
        "imm": IMMS,
        "lab": LABS,
        "big": BIG,
    }[var_type(var)]


def u32_not(val):
    return U32_1 ^ val


def gen_instruction(instruction, assignments):
    for k, v in assignments.items():
        instruction = instruction.replace(k, v)
    if any(var_type(inst) == "lab" for inst in assignments.keys()):
        instruction = f"before: {instruction}; after:"
    return instruction


def find_changed(values):
    always_0 = U32_1
    always_1 = U32_1
    for v in values:
        always_0 &= u32_not(v)
        always_1 &= v
    return u32_not(always_0 | always_1)


def var_name(var):
    return var[5:-1]


def var_type(var):
    return var[1:4]


def find_offset(values):
    str_repr = f"{find_changed(values):b}"
    start = len(str_repr)
    end = len(str_repr.replace("1", ""))
    return (start, end)


default_assignments = {v: var_options(v)[0] for v in vars}

offsets = {}
all_values = []
for i in range(len(vars)):
    var = vars[i]
    values = []
    for val in var_options(var):
        assignments = {v: var_options(v)[0] if v != var else val for v in vars}
        instruction = gen_instruction(inst, assignments)
        values.append(encoding(instruction))
    _low, high = find_offset(values)
    all_values.extend(values)
    offsets[var] = high

opcode = all_values[0] & u32_not(find_changed(all_values))

print(
    f"static inline uint32_t op({', '.join('uint32_t ' + var_name(var) for var in vars)}) {{\n"
    f"    return 0x{opcode:x} | {' | '.join(('(' + var_name(k) + ' << ' + str(v) + ')') for k, v in offsets.items())};\n"
    f"}}"
)
