import 'dart:typed_data';
import 'package:test/test.dart';
import '../lib/template.dart';
import '../lib/assembly.dart';
import '../lib/bytecode.dart';

void main() {
  group('Template loading', () {
    test('loads valid template file', () async {
      Template t = await Template.load('test/assembly.image');
      expect(t.frames.length, greaterThan(0));
      expect(t.inputs.length, greaterThan(0));
    });

    test('throws on invalid header', () async {
      expect(
        () async => await Template.load('test/invalid.image'),
        throwsException,
      );
    });
  });

  group('Interpreter', () {
    test('constant opcode sets register 0', () {
      Uint8List bc = Uint8List.fromList([
        OpCode.constant,
        0x00,
        255,
        0,
        0,
        255,
        OpCode.ret,
      ]);

      Template t = Template(0, [Input(1, 1)], [Frame(1, 1, 0, bc, bc.length)]);

      ImgasmImage input = ImgasmImage(1, 1, Uint8List(4));
      ImgasmImage result = Interpreter.assemble(t, 0, [input]);

      expect(result.pixels[0], equals(255)); // R
      expect(result.pixels[1], equals(0));
      expect(result.pixels[2], equals(0));
    });

    test('move opcode copies register', () {
      Uint8List bc = Uint8List.fromList([
        OpCode.constant,
        0x10,
        100,
        150,
        200,
        255,
        OpCode.move,
        0x01,
        OpCode.ret,
      ]);

      Template t = Template(0, [Input(1, 1)], [Frame(1, 1, 0, bc, bc.length)]);

      ImgasmImage input = ImgasmImage(1, 1, Uint8List(4));
      ImgasmImage result = Interpreter.assemble(t, 0, [input]);

      expect(result.pixels[0], equals(100));
      expect(result.pixels[1], equals(150));
      expect(result.pixels[2], equals(200));
    });

    test('add opcode adds registers', () {
      Uint8List bc = Uint8List.fromList([
        OpCode.constant,
        0x00,
        100,
        0,
        0,
        0,
        OpCode.constant,
        0x10,
        50,
        0,
        0,
        0,
        OpCode.add,
        0x01,
        OpCode.ret,
      ]);

      Template t = Template(0, [Input(1, 1)], [Frame(1, 1, 0, bc, bc.length)]);

      ImgasmImage input = ImgasmImage(1, 1, Uint8List(4));
      ImgasmImage result = Interpreter.assemble(t, 0, [input]);

      expect(result.pixels[0], equals(150));
    });

    test('10+ operations', () {
      Uint8List bc = Uint8List.fromList([
        OpCode.constant,
        0x00,
        10,
        0,
        0,
        0,
        OpCode.constant,
        0x10,
        10,
        0,
        0,
        0,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.add,
        0x01,
        OpCode.ret,
      ]);

      Template t = Template(0, [Input(1, 1)], [Frame(1, 1, 0, bc, bc.length)]);

      ImgasmImage input = ImgasmImage(1, 1, Uint8List(4));
      ImgasmImage result = Interpreter.assemble(t, 0, [input]);

      expect(result.pixels[0], equals(110)); // 10 + 10*10
    });

    test('sample opcode reads pixel from input', () {
      Uint8List inputData = Uint8List.fromList([0, 0, 255, 255]);
      ImgasmImage inputImg = ImgasmImage(1, 1, inputData);

      Uint8List bc = Uint8List.fromList([
        OpCode.sample, 0x00, 0, 0, 0, 0, // Reg0 = Input[0].pixel(0,0)
        OpCode.ret,
      ]);

      Template t = Template(0, [Input(1, 1)], [Frame(1, 1, 0, bc, bc.length)]);
      ImgasmImage result = Interpreter.assemble(t, 0, [inputImg]);

      expect(result.pixels[2], equals(255)); // B
    });

    test('throws on wrong input count', () {
      Uint8List bc = Uint8List.fromList([OpCode.ret]);
      Template t = Template(
        0,
        [Input(1, 1), Input(1, 1)],
        [Frame(1, 1, 0, bc, bc.length)],
      );

      ImgasmImage input = ImgasmImage(1, 1, Uint8List(4));

      expect(() => Interpreter.assemble(t, 0, [input]), throwsException);
    });
  });
}
