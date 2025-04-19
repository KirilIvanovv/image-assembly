package imageassembly

import (
	"encoding/binary"
	"io"
	"os"
	"fmt"
)

func Add(a, b int) int {
	return a + b + 420
}

var verbose bool = true

func Debugln(v ...any) {
	if verbose {
		fmt.Println(v...)
	}
}

type AssemblyError struct {
	Code    int
	Message string
}

func (e *AssemblyError) Error() string {
	return fmt.Sprintf("%s (Error Code %d)", e.Message, e.Code)
}

const (
	ErrorFileIOError      = iota
	ErrorFileOpenError

	ErrorFileIncorrectHeader
	ErrorFileIncorrectVersion
	ErrorFileIncorrectInputInfo
)

/*

class ImageAssembly
class Frame
class Pixel
class Operation

class ImageSource
- load (file)
- save (file)

*/

type AssemblyInput struct {
	Width int
	Height int
}

type ImageAssembly struct {
	input_count int
	frame_count int

	flags int

	inputs []AssemblyInput
}

func LoadImageAssembly(filename string) (*ImageAssembly, error) {
	// ============================= OPENING FILE =============================

	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	// ============================ PARSING HEADER ============================
	var file_header [8]byte
	_, err = io.ReadFull(f, file_header[:])
	if err != nil {
		return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
	}

	if string(file_header[:]) != "IMGBCODE" {
		return nil, &AssemblyError {Code: ErrorFileIncorrectHeader, Message: fmt.Sprintf("Invalid file header '%s' in '%s'", string(file_header[:]), filename)}
	}

	Debugln("Opened file:", filename)

	var file_type_version uint16
	var file_flags uint16
	var input_count uint16
	var frame_count uint16

	err = binary.Read(f, binary.LittleEndian, &file_type_version)
	if (err != nil) {
		return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
	}

	err = binary.Read(f, binary.LittleEndian, &file_flags)
	if (err != nil) {
		return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
	}

	err = binary.Read(f, binary.LittleEndian, &input_count)
	if (err != nil) {
		return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
	}

	err = binary.Read(f, binary.LittleEndian, &frame_count)
	if (err != nil) {
		return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
	}

	Debugln("File type:", file_type_version)
	Debugln("File flags:", file_flags)
	Debugln("File inputs:", input_count)
	Debugln("File frames:", frame_count)

	if file_type_version > 0 {
		return nil, &AssemblyError {Code: ErrorFileIncorrectVersion, Message: fmt.Sprintf("This library version can't process files with version '%d'", file_type_version)}
	}

	if int(input_count) < 1 {
		return nil, &AssemblyError {Code: ErrorFileIncorrectInputInfo, Message: fmt.Sprintf("File contains %d inputs, but needs at least 1", input_count)}
	}

	inputs := make([]AssemblyInput, input_count)

	for i := 0; i < int(input_count); i++ {
		var input_index uint8
		err = binary.Read(f, binary.LittleEndian, &input_index)
		if (err != nil) {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
		}

		if (uint(input_index) > uint(input_count)) {
			return nil, &AssemblyError {Code: ErrorFileIncorrectInputInfo, Message: fmt.Sprintf("Incorrent input index %d in '%s'", input_index, filename)}
		}

		_, err = f.Seek(3, io.SeekCurrent)
		if err != nil {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
		}

		var input_width uint16
		var input_height uint16

		err = binary.Read(f, binary.LittleEndian, &input_width)
		if (err != nil) {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
		}

		err = binary.Read(f, binary.LittleEndian, &input_height)
		if (err != nil) {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file header for '%s'", filename)}
		}

		Debugln("Input index, width, height:", input_index, input_width, input_height)

		inputs[input_index].Width = int(input_width)
		inputs[input_index].Height = int(input_height)
	}
	

	// TODO:
	//	- go through the file
	//	- find frame headers
	//  - parse frame headers
	//  - remember the bytecode somewhere


	return &ImageAssembly{inputs: inputs}, nil
}

func (this *ImageAssembly) GetWidth() int {
	return this.inputs[0].Width;
}

func (this *ImageAssembly) GetHeight() int {
	return this.inputs[0].Height;
}
