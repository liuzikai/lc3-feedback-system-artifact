export function uint8arrayToString(myUint8Arr) {
    return String.fromCharCode.apply(null, myUint8Arr);
}

// source: https://stackoverflow.com/questions/18729405/how-to-convert-utf8-string-to-byte-array
export function toUTF8Array(str) {
    var utf8 = [];
    for (var i = 0; i < str.length; i++) {
        var charcode = str.charCodeAt(i);
        if (charcode < 0x80) utf8.push(charcode);
        else if (charcode < 0x800) {
            utf8.push(0xc0 | (charcode >> 6), 0x80 | (charcode & 0x3f));
        } else if (charcode < 0xd800 || charcode >= 0xe000) {
            utf8.push(0xe0 | (charcode >> 12), 0x80 | ((charcode >> 6) & 0x3f), 0x80 | (charcode & 0x3f));
        } else {
            i++;
            charcode = 0x10000 + (((charcode & 0x3ff) << 10) | (str.charCodeAt(i) & 0x3ff));
            utf8.push(0xf0 | (charcode >> 18), 0x80 | ((charcode >> 12) & 0x3f), 0x80 | ((charcode >> 6) & 0x3f), 0x80 | (charcode & 0x3f));
        }
    }
    return utf8;
}

export function toUTF8StrArray(str) {
    let utf8 = toUTF8Array(str);
    utf8.push(0);
    return utf8;
}

export function ptr2str(Module, ptr) {
    const stringLen = Module.cwrap("stringLen", "number", ["number"]);
    const length = stringLen(ptr);
    const strArray = new Int8Array(Module.HEAP8.buffer, ptr, length);
    const res = uint8arrayToString(strArray).trim();
    // console.log(res)
    return res;
}

export function runlc3as(Module, input_asm) {
    const FS = Module.FS;
    const lc3as = Module.cwrap("main_lc3as", "number", ["number"]);
    const input_file_name = "input.asm";
    const converted_str = new Uint8Array(toUTF8Array(input_asm));
    FS.writeFile(input_file_name, converted_str);
    // alloc memory
    const file_str = toUTF8StrArray(input_file_name)
    const input_ptr = Module._malloc(file_str.length * 1); // 1 byte per element (left just to see)
    Module.HEAPU8.set(file_str, input_ptr); // write WASM memory calling the set method of the Uint8Array
    const ret = lc3as(input_ptr); // call the WASM function

    // dealloc memory
    Module._free(input_ptr);

    return {
        ret,
        sym: FS.readFile("input.sym"),
        obj: FS.readFile("input.obj"),
        debug: FS.readFile("input.debug")
    }
}

export function disasOne(Module, addr) {
    const disassemble_one = Module.cwrap("disassemble_one_export", "number", ["number", "number", "number", "number"]);
    const label = Module._malloc(100);
    const op = Module._malloc(100);
    const operands = Module._malloc(100);
    const inst = disassemble_one(addr, label, op, operands); // call the WASM function
    const result = {
        label: ptr2str(Module, label),
        op: ptr2str(Module, op),
        operands: ptr2str(Module, operands).split(" "),
        inst
    }
    
    Module._free(label);
    Module._free(op);
    Module._free(operands);
    return result;
}

export function runlc3sim(Module, fileName) {
    // const FS = Module.FS;
    const lc3sim = Module.cwrap("main_lc3sim", "number", ["number"]);
    // FS.writeFile("input.sym", lc3asResult.sym);
    // FS.writeFile("input.obj", lc3asResult.obj);
    // FS.writeFile("input.debug", lc3asResult.debug);
    const input_file_name = fileName.join(" ");
    console.log("input_file_name", input_file_name)

    const file_str = toUTF8StrArray(input_file_name)
    const input_ptr = Module._malloc(file_str.length * 1);
    Module.HEAPU8.set(file_str, input_ptr);
    console.log("before call")
    const ret = lc3sim(input_ptr);
    // dealloc memory
    Module._free(input_ptr);

    return {
        ret
    };
}