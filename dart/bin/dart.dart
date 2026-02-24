import 'dart:typed_data';
import 'dart:io';
import 'package:image/image.dart' as img;
import '../lib/template.dart';
import '../lib/bytecode.dart';
import '../lib/assembly.dart';

void main() async 
{
    print("--- Starting image assembly ---");

    String templatePath = 'test/assembly.image';
    String inputPath    = 'test/test1.png';
    String outputPath   = 'test/out.png';

    try 
    {
        print("Loading template: $templatePath...");
        Template template = await Template.load(templatePath);

        print("Loading image: $inputPath...");

        ImgasmImage outputBuffer = ImgasmImage(
            template.frames[0].width,
            template.frames[0].height,
            Uint8List(template.frames[0].width * template.frames[0].height * 4),
        );

        ImgasmImage inputImg = loadExternalImage(inputPath);

        print("Resizing images to match template requirements...");

        if (inputImg.width != template.inputs[1].width || 
            inputImg.height != template.inputs[1].height) 
        {
            print("Resizing input image to ${template.inputs[1].width}x${template.inputs[1].height}");
            inputImg = resizeImgasm(inputImg, template.inputs[1].width, template.inputs[1].height);
        }

        inputImg = flipVertical(inputImg);

        print("Assembling frame...");
        ImgasmImage result = Interpreter.assemble(template, 0, [outputBuffer, inputImg]);

        print("Saving result to $outputPath...");
        img.Image outImage = img.Image.fromBytes(
            width:       result.width,
            height:      result.height,
            bytes:       result.pixels.buffer,
            bytesOffset: result.pixels.offsetInBytes,
            numChannels: 4,
        );

        File(outputPath).writeAsBytesSync(img.encodePng(outImage));
        print("--- Done! ---");
        print("Output: ${Directory.current.path}/$outputPath");
    } 
    catch (e, stack) 
    {
        print("ERROR:");
        print(e);
        print(stack);
    }
}

ImgasmImage loadExternalImage(String path) 
{
    final Uint8List fileBytes = File(path).readAsBytesSync();
    img.Image? decoded = img.decodeImage(fileBytes);

    if (decoded == null) 
    {
        throw Exception("Failed to decode image: $path");
    }

    final img.Image rgba = decoded.numChannels == 4
        ? decoded
        : decoded.convert(numChannels: 4);

    final Uint8List pixels = rgba.getBytes();

    return ImgasmImage(rgba.width, rgba.height, pixels);
}

ImgasmImage resizeImgasm(ImgasmImage src, int targetW, int targetH) 
{
    img.Image temp = img.Image.fromBytes(
        width:       src.width, 
        height:      src.height, 
        bytes:       src.pixels.buffer,
        bytesOffset: src.pixels.offsetInBytes,
        numChannels: 4,
    );

    img.Image resized = img.copyResize(temp, width: targetW, height: targetH);

    final Uint8List pixels = resized.getBytes();
    return ImgasmImage(targetW, targetH, pixels);
}

ImgasmImage flipVertical(ImgasmImage src) 
{
    img.Image temp = img.Image.fromBytes(
        width:       src.width,
        height:      src.height,
        bytes:       src.pixels.buffer,
        bytesOffset: src.pixels.offsetInBytes,
        numChannels: 4,
    );

    img.Image flipped = img.flipVertical(temp);

    return ImgasmImage(src.width, src.height, flipped.getBytes());
}