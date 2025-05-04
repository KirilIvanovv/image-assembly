package main

import (
	"fmt"
	. "github.com/racenis/imageassembly"
	"image"
    //"image/color"
    "image/png"
    "os"

	//"github.com/nfnt/resize"
	//"path/filepath"
	"log"
	"image/draw"
	"github.com/disintegration/imaging"
)

func main() {

	// Loads image assembly file
	assembly, err := LoadImageAssembly("assembly.image")

	if err != nil {
		fmt.Println("FILE DIDNT OPEEN!")
		fmt.Println("Error:", err)
		return
	}

	// Prints some info about the assembly
	assembly.PrintInfo()

	// Prints bytecode for a specific pixel
	//assembly.PrintBytecode(0, 100, 120)


	// Retrieving input template
	template := assembly.GetTemplate()

	
	filePath := "test1.png" // change to your image path
    file, err := os.Open(filePath)
    if err != nil {
        //return nil, err
        return
    }
    defer file.Close()

    //ext := filepath.Ext(filePath)
    //switch ext {
    //case ".png":
        img, err := png.Decode(file)
     //   break
    /*case ".jpg", ".jpeg":
        return jpeg.Decode(file)*/
    /*default:
       
        img := nil
        fmt.Errorf("unsupported file type: %s", ext)
    }*/


	if err != nil {
		log.Fatal(err)
	}

	resizedImg := imaging.Resize(img, int(template.Images[1].Width), int(template.Images[1].Height), imaging.Lanczos)


    bounds := resizedImg.Bounds()
    rgbaImg := image.NewRGBA(bounds)
    draw.Draw(rgbaImg, bounds, resizedImg, bounds.Min, draw.Src)


	byteArray := rgbaImg.Pix


	template.Images[1].Pixels = byteArray

    for i := 0; i < assembly.GetFrameCount(); i++ {

        fmt.Println("Rendering frame: %d", i)

        // Generate new image
        assembly.AssembleImage(i, template)






        // Write out the new image to disk
        rgba := &image.RGBA {
            Pix:    template.Images[0].Pixels,
            Stride: template.Images[0].Width * 4,
            Rect:   image.Rect(0, 0, template.Images[0].Width, template.Images[0].Height),
        }

        // Create the output file
        outFile, err := os.Create(fmt.Sprintf("chungus%d.png", i))
        if err != nil {
            return //err
        }
        defer outFile.Close()


        png.Encode(outFile, rgba)
    }

}