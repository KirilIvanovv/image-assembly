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
	ErrorFileIncorrectFrame

	ErrorInvalidOperation
)


const (
	opNoop = 0
	opMove = 2
	opReturn = 3
	opConstant = 4
	opSample = 5
	opAdd = 8
	opMultiply = 9
)

func op_length(operation uint8) int {
	switch operation {
		case opNoop:		return 0
		case opMove:		return 1
		case opReturn:		return 0
		case opConstant:	return 5
		case opSample:		return 5
		case opAdd:			return 1
		case opMultiply:	return 1
		default:			return -1
	}
}

func op_name(operation uint8) string {
	switch operation {
		case opNoop:		return "NOOP"
		case opMove:		return "MOVE"
		case opReturn:		return "RET "
		case opConstant:	return "LOAD"
		case opSample:		return "SMPL"
		case opAdd:			return "ADD "
		case opMultiply:	return "MULT"
		default:			return "INVD"
	}
}

type AssemblyInput struct {
	Width int
	Height int
}

type AssemblyFrame struct {
	Width int
	Height int
	Bytecode []byte
}

type ImageAssembly struct {
	input_count int
	frame_count int

	flags int

	inputs []AssemblyInput
	frames []AssemblyFrame
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
	frames := make([]AssemblyFrame, frame_count)

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
	
	for i := 0; i < int(frame_count); i++ {
		var frame_header [4]byte
		_, err = io.ReadFull(f, frame_header[:])
		if err != nil {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame header for '%s'", filename)}
		}

		if string(frame_header[:]) != "FRME" {
			return nil, &AssemblyError {Code: ErrorFileIncorrectFrame, Message: fmt.Sprintf("Invalid frame header '%s' in '%s'", string(frame_header[:]), filename)}
		}

		var frame_width uint16
		var frame_height uint16

		err = binary.Read(f, binary.LittleEndian, &frame_width)
		if (err != nil) {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame in '%s'", filename)}
		}

		err = binary.Read(f, binary.LittleEndian, &frame_height)
		if (err != nil) {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame in '%s'", filename)}
		}

		pixel_count := int(frame_width) * int(frame_height)

		Debugln("Frame index:", i)
		Debugln("Frame dimensions:", frame_width, "by", frame_height)

		bytecode_begin, err := f.Seek(0, io.SeekCurrent)
		if err != nil {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame in '%s'", filename)}
		}

		for j := 0; j < pixel_count; j++ {
			for {
				var operation uint8
				
				err = binary.Read(f, binary.LittleEndian, &operation)
				if err != nil {
					return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read operation in '%s'", filename)}
				}

				parameter_length := int64(op_length(operation))
				if parameter_length < 0 {
					return nil, &AssemblyError {Code: ErrorInvalidOperation, Message: fmt.Sprintf("Invalid operation %x '%s'", operation, filename)}
				}

				if parameter_length > 0 {
					_, err = f.Seek(parameter_length, io.SeekCurrent)
					if err != nil {
						return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read file frame in '%s'", filename)}
					}
				}

				if operation == opReturn {
					break
				}

			}
		}

		bytecode_end, err := f.Seek(0, io.SeekCurrent)
		if err != nil {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame in '%s'", filename)}
		}

		bytecode_length := bytecode_end - bytecode_begin
		bytecode := make([]byte, bytecode_length)

		_, err = f.ReadAt(bytecode, bytecode_begin)
		if err != nil {
			return nil, &AssemblyError {Code: ErrorFileIOError, Message: fmt.Sprintf("Can't read frame in '%s'", filename)}
		}

		frames[i].Width = int(frame_width)
		frames[i].Height = int(frame_height)
		frames[i].Bytecode = bytecode
	}


	return &ImageAssembly{flags: int(file_flags), input_count: int(input_count), frame_count: int(frame_count), inputs: inputs, frames: frames}, nil
}

func (this *ImageAssembly) GetWidth() int {
	return this.inputs[0].Width;
}

func (this *ImageAssembly) GetHeight() int {
	return this.inputs[0].Height;
}

func (this *ImageAssembly) PrintInfo() {
	fmt.Println("Image assembly")
	fmt.Println("Flags:", this.flags)
	fmt.Println("Inputs:", this.input_count)
	for i := 0; i < this.input_count; i++ {
		fmt.Println("\tInput", i, "size", this.inputs[i].Width, "by", this.inputs[i].Height)
	}
	fmt.Println("Frames:", this.frame_count)
	for i := 0; i < this.frame_count; i++ {
		fmt.Println("\tFrame", i, "size", this.frames[i].Width, "by", this.frames[i].Height, "with", len(this.frames[i].Bytecode), "bytecodes")
	}
}

func (this *ImageAssembly) PrintBytecode(frame int, x int, y int) {
	if frame < 0 || frame >= this.frame_count {
		fmt.Println("Invalid frame index", frame, ", index allowed between 0 and", this.frame_count)
	}

	if x < 0 || x >= this.frames[frame].Width {
		fmt.Println("Invalid frame", frame, "x coordinate", x, ", coordinate allowed between 0 and", this.frames[frame].Width)
	}

	if y < 0 || y >= this.frames[frame].Height {
		fmt.Println("Invalid frame", frame, "y coordinate", y, ", coordinate allowed between 0 and", this.frames[frame].Height)
	}

	pixel_index := this.frames[frame].Width * y + x
	byte_index := 0

	for i := 0; i < pixel_index; i++ {
		for {
			operation := uint8(this.frames[frame].Bytecode[byte_index])

			parameter_length := op_length(operation)

			if parameter_length < 0 {
				fmt.Println("Wait what")
			}

			byte_index += parameter_length + 1

			if operation == opReturn {
				break
			}

		}
		
	}

	fmt.Println("Found at index:", byte_index)

	for {
		fmt.Printf("%06d ", byte_index)

		operation := this.frames[frame].Bytecode[byte_index]
		byte_index++

		switch operation {
			case opNoop:
				fmt.Print("NOOP")
				break
			case opMove:
				param := this.frames[frame].Bytecode[byte_index]
				byte_index++
				
				destination_register := param >> 4 
				source_register := param & 0x0F

				fmt.Printf("MOVE %02d, %02d", destination_register, source_register)

				break
			case opReturn:
				fmt.Print("RET ")
				break
			case opConstant:

				param := this.frames[frame].Bytecode[byte_index]
				byte_index++
				
				//destination_register := param & 0x0F
				destination_register := param >> 4

				r := this.frames[frame].Bytecode[byte_index + 0]
				g := this.frames[frame].Bytecode[byte_index + 1]
				b := this.frames[frame].Bytecode[byte_index + 2]
				a := this.frames[frame].Bytecode[byte_index + 3]

				byte_index += 4

				fmt.Printf("LOAD %02d, %02x %02x %02x %02x", destination_register, r, g, b, a)
				
				break
			case opSample:
				
				param := this.frames[frame].Bytecode[byte_index]
				byte_index++

				destination_register := param >> 4 
				source_input := param & 0x0F


				x := binary.LittleEndian.Uint16(this.frames[frame].Bytecode[byte_index + 0 : byte_index + 2])
				y := binary.LittleEndian.Uint16(this.frames[frame].Bytecode[byte_index + 2 : byte_index + 4])

				byte_index += 4

				fmt.Printf("SMPL %02d, %02d %04d %04d", destination_register, source_input, x, y)

				break
			case opAdd:
				param := this.frames[frame].Bytecode[byte_index]
				byte_index++
				
				destination_register := param >> 4 
				source_register := param & 0x0F

				fmt.Printf("ADD  %02d, %02d", destination_register, source_register)
				break
			case opMultiply:
				param := this.frames[frame].Bytecode[byte_index]
				byte_index++
				
				destination_register := param >> 4 
				source_register := param & 0x0F

				fmt.Printf("MULT %02d, %02d", destination_register, source_register)
				break
			default:
				fmt.Print("INVD")
		}

		fmt.Print("\n")

		if operation == opReturn {
			break
		}
	}
}
