package main

import (
	"fmt"
	. "github.com/racenis/imageassembly"
)

func main() {

	fmt.Println("I am a frog!")

	image, err := LoadImageAssembly("assembly.image")

	if err != nil {
		fmt.Println("FILE DIDNT OPEEN!")
		fmt.Println("Error:", err)
		return
	}

	

	fmt.Println("here:", image.GetWidth(), "by", image.GetHeight())

	image.PrintInfo()

	image.PrintBytecode(0, 21, 15)
	//for i := 0 ; i < 30 ; i++ {
	//	image.PrintBytecode(0, i, 15)
	//}
}