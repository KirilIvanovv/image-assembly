import 'dart:typed_data';
import 'template.dart';
import 'bytecode.dart';

class ImgasmImage
{
    int width;
    int height;
    Uint8List pixels;

    ImgasmImage(this.width, this.height, this.pixels);
}

class Interpreter 
{
  static ImgasmImage assemble(Template template, int frameIndex, List<ImgasmImage> inputs) 
  {
    if (frameIndex < 0 || frameIndex >= template.frames.length) 
    {
      throw Exception("Invalid frame index: $frameIndex");
    }

    if (inputs.length != template.inputs.length) 
    {
      throw Exception("Invalid input count: expected ${template.inputs.length}, got ${inputs.length}");
    }

    Frame frame = template.frames[frameIndex];

    Uint8List outputPixels = Uint8List(frame.width * frame.height * 4);

    final List<Int32List> registers = List.generate(16, (_) => Int32List(4));


    int byteIndex = 0; 
    Uint8List bc = frame.bytecode!;
    int totalPixels = frame.width * frame.height;

    for (int p = 0; p < totalPixels; p++) 
    {
        for (var reg in registers) 
        {
            reg[0] = reg[1] = reg[2] = reg[3] = 0;
        }

        while (byteIndex < bc.length)
        {
            int operation = bc[byteIndex++];

            if (operation == OpCode.ret) break; 
            switch (operation) 
            {
            case OpCode.noop:
                break;

            case OpCode.move:
                int param = bc[byteIndex++];
                int dst = param >> 4;
                int src = param & 0x0F;
                registers[dst][0] = registers[src][0];
                registers[dst][1] = registers[src][1];
                registers[dst][2] = registers[src][2];
                registers[dst][3] = registers[src][3];
                break;

            case OpCode.constant:
                int param = bc[byteIndex++];
                int dst = param >> 4;
                registers[dst][0] = bc[byteIndex++];
                registers[dst][1] = bc[byteIndex++];
                registers[dst][2] = bc[byteIndex++];
                registers[dst][3] = bc[byteIndex++];
                break;

            case OpCode.sample:
                int param = bc[byteIndex++];
                int dst = param >> 4;
                int inputIdx = param & 0x0F;
                int sx = bc[byteIndex++] | (bc[byteIndex++] << 8);
                int sy = bc[byteIndex++] | (bc[byteIndex++] << 8);

                if (inputIdx < inputs.length) {
                ImgasmImage srcImg = inputs[inputIdx];
                if (sx >= 0 && sx < srcImg.width && sy >= 0 && sy < srcImg.height) {
                    int srcIdx = (sy * srcImg.width + sx) * 4;
                    registers[dst][0] = srcImg.pixels[srcIdx + 0];
                    registers[dst][1] = srcImg.pixels[srcIdx + 1];
                    registers[dst][2] = srcImg.pixels[srcIdx + 2];
                    registers[dst][3] = 255;
                }
                }
                break;

            case OpCode.add:
                int param = bc[byteIndex++];
                int dst = param >> 4;
                int src = param & 0x0F;
                registers[dst][0] += registers[src][0];
                registers[dst][1] += registers[src][1];
                registers[dst][2] += registers[src][2];
                break;

            case OpCode.multiply:
                int param = bc[byteIndex++];
                int dst = param >> 4;
                int src = param & 0x0F;
                registers[dst][0] = (registers[dst][0] * registers[src][0]) >> 8;
                registers[dst][1] = (registers[dst][1] * registers[src][1]) >> 8;
                registers[dst][2] = (registers[dst][2] * registers[src][2]) >> 8;

                break;
                
            default:
                break;
            }
        }

        int outIdx = p * 4;
        outputPixels[outIdx + 0] = registers[0][0].toUnsigned(8);
        outputPixels[outIdx + 1] = registers[0][1].toUnsigned(8);
        outputPixels[outIdx + 2] = registers[0][2].toUnsigned(8);
        outputPixels[outIdx + 3] = 255; 
        }


    return ImgasmImage(frame.width, frame.height, outputPixels);
  }
}