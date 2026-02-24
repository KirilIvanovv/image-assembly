import 'dart:typed_data';
import 'dart:io';
import 'bytecode.dart';

class Frame 
{
    int width;
    int height;
    int offset;
    Uint8List? bytecode;
    int bytecodeLength;

    Frame(this.width, this.height, this.offset, this.bytecode, this.bytecodeLength);
}

class Input
{
    int width;
    int height;

    Input(this.width, this.height);
}

class Template
{
    int flags;
    List<Input> inputs; 
    List<Frame> frames;

    Template(this.flags, this.inputs, this.frames);

    static Future<Template> load(String filepath) async 
    {
        final file = File(filepath);
        final Uint8List bytes = await file.readAsBytes();
        final ByteData data = ByteData.view(bytes.buffer);
        int offset = 0;

        // ============================= LOAD HEADER =============================
        if (String.fromCharCodes(bytes.sublist(0, 8)) != "IMGBCODE")
        {
            throw Exception("Invalid file header");
        }
        offset += 8;

        int version = data.getUint16(offset, Endian.little);
        offset += 2;
        int flag = data.getUint16(offset, Endian.little);
        offset += 2;
        int inputCount = data.getUint16(offset, Endian.little);
        offset += 2;
        int frameCount = data.getUint16(offset, Endian.little);
        offset += 2; 

        if (version < 0 || version > 1)
        {
            throw Exception("Unsupported version: $version");
        }

        if (inputCount < 1)
        {
            throw Exception("File contains $inputCount inputs, but needs at least 1");
        }

        // ========================== LOAD INPUT INFOS ===========================
        List<Input> inputs = [];

        for (int i = 0; i < inputCount; i++)
        {
            int inputIndex = data.getUint8(offset);
            offset += 1;

            offset += 3;

            int width = data.getUint16(offset, Endian.little);
            offset += 2;

            int height = data.getUint16(offset, Endian.little);
            offset += 2;

            inputs.add(Input(width, height));
        }

        // ========================== LOAD FRAME INFOS ===========================
        List<Frame> frames = [];

        for (int i = 0; i < frameCount; i++) 
        {
            offset += 4;

            int fWidth = data.getUint16(offset, Endian.little);
            offset += 2;
            int fHeight = data.getUint16(offset, Endian.little);
            offset += 2;

            int bcLen;
            if (version >= 1) 
            {
                bcLen = data.getUint32(offset, Endian.little);
                offset += 4;
            } 
            else 
            {
                int startBc = offset;
                int pixelCount = fWidth * fHeight;
                for (int p = 0; p < pixelCount; p++) 
                {
                    while (true) 
                    {
                        int op = bytes[offset];
                        int pLen = OpCode.getLength(op);
                        offset += 1 + pLen;
                        if (op == OpCode.ret) break;
                    }
            }
            bcLen = offset - startBc;
            offset = startBc; 
            }

            Uint8List bc = bytes.sublist(offset, offset + bcLen);
            offset += bcLen; 

            frames.add(Frame(fWidth, fHeight, offset - bcLen, bc, bcLen));
        }

        return Template(flag, inputs, frames);
    }
    
    void printBytecode(int frameIndex, int x, int y) 
    {
        if (frameIndex < 0 || frameIndex >= frames.length) 
        {
            print("Invalid frame index");
            return;
        }
        Frame frame = frames[frameIndex];
        Uint8List? bc = frame.bytecode;
        if (bc == null) return;

        int pixelIndex = frame.width * y + x;
        int byteIndex = 0;

        for (int i = 0; i < pixelIndex; i++) 
        {
            while (true) 
            {
                int op = bc[byteIndex];
                int paramLen = OpCode.getLength(op);
                byteIndex += paramLen + 1;
                if (op == OpCode.ret) break;
            }
        }

        print("Found at index: $byteIndex");

        while (true) 
        {
            String offsetStr = byteIndex.toString().padLeft(6, '0');
            int op = bc[byteIndex];
            String name = OpCode.getName(op);
            stdout.write("$offsetStr $name ");
            byteIndex++;

            switch (op) 
            {
                case OpCode.move:
                case OpCode.add:
                case OpCode.multiply:
                    int p = bc[byteIndex];
                    print("${(p >> 4).toString().padLeft(2, '0')}, ${(p & 0x0F).toString().padLeft(2, '0')}");
                    byteIndex += 1;
                    break;
                case OpCode.constant:
                    int p = bc[byteIndex];
                    String r = bc[byteIndex + 1].toRadixString(16).padLeft(2, '0');
                    String g = bc[byteIndex + 2].toRadixString(16).padLeft(2, '0');
                    String b = bc[byteIndex + 3].toRadixString(16).padLeft(2, '0');
                    String a = bc[byteIndex + 4].toRadixString(16).padLeft(2, '0');
                    print("${(p >> 4).toString().padLeft(2, '0')}, $r $g $b $a");
                    byteIndex += 5;
                    break;
                case OpCode.sample:
                    int p = bc[byteIndex];
                    ByteData view = ByteData.view(bc.buffer, bc.offsetInBytes + byteIndex + 1, 4);
                    int sx = view.getUint16(0, Endian.little);
                    int sy = view.getUint16(2, Endian.little);
                    print("${(p >> 4).toString().padLeft(2, '0')}, ${(p & 0x0F).toString().padLeft(2, '0')} $sx $sy");
                    byteIndex += 5;
                    break;
                default:
                    print("");
                    break;
            }

            if (op == OpCode.ret) break;
        }
    }
}