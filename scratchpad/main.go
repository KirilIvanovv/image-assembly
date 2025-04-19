package main

import (
	"fmt"
	. "github.com/racenis/imageassembly"
)

func main() {
	image, err := LoadImageAssembly("assembly.image")

	if err != nil {
		fmt.Println("FILE DIDNT OPEEN!")
		fmt.Println("Error:", err)
		return
	}

	fmt.Println("here:", image.GetWidth(), "by", image.GetHeight())
}