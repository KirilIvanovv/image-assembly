package main

import (
	"fmt"
	. "github.com/racenis/imageassembly"
	"image"
    //"image/color"
    "image/png"
    "os"

	//"github.com/nfnt/resize"
	"path/filepath"
	"log"
	"image/draw"
	"github.com/disintegration/imaging"
)

func loadImage(filename string) (image.Image, error) {
    file, err := os.Open(filename)
    if err != nil {
        return nil, err
    }
    defer file.Close()

    ext := filepath.Ext(filename)
    switch ext {
    case ".png":
        return png.Decode(file)
    /*case ".jpg", ".jpeg":
        return jpeg.Decode(file)*/
    default:
        return nil, fmt.Errorf("unsupported file type: %s", ext)
    }
}

// toRGBA converts any image.Image to *image.RGBA
func toRGBA(img image.Image) *image.RGBA {
    bounds := img.Bounds()
    rgba := image.NewRGBA(bounds)
    draw.Draw(rgba, bounds, img, bounds.Min, draw.Src)
    return rgba
}

func main() {

	fmt.Println("I am a frog!")

	assembly, err := LoadImageAssembly("assembly.image")

	if err != nil {
		fmt.Println("FILE DIDNT OPEEN!")
		fmt.Println("Error:", err)
		return
	}

	

	fmt.Println("here:", assembly.GetWidth(), "by", assembly.GetHeight())

	assembly.PrintInfo()

	assembly.PrintBytecode(0, 171, 92)
	//for i := 0 ; i < 30 ; i++ {
	//	image.PrintBytecode(0, i, 15)
	//}

	/*type AssemblyImage struct {
		Width int
		Height int
		Pixels []byte
	}
	
	type AssemblyList struct {
		Images []AssemblyImage
	}*/


	/*output_buffer := AssemblyImage {
		Width: image.GetWidth(),
		Height: image.GetHeight()
		Pixels: make([]byte, image.GetWidth() * image.GetHeight() * 4)
	}*/

	template := assembly.GetTemplate()

	






	// Open image file
	/*file, err := os.Open("test1.png")
	if err != nil {
		return nil, 0, 0, err
	}
	defer file.Close()

	var img image.Image
    switch {
    case strings.HasSuffix(path, ".png"):
        img, err = png.Decode(file)
    case strings.HasSuffix(path, ".jpg"), strings.HasSuffix(path, ".jpeg"):
        img, err = jpeg.Decode(file)
    default:
        return nil, 0, 0, fmt.Errorf("unsupported file format")
    }
    if err != nil {
        return nil, 0, 0, err
    }*/

	/*file, err := os.Open("input.png")
	if err != nil {
		panic(err)
	}
	defer file.Close()

	// Decode image
	img, _, err := image.Decode(file)
	if err != nil {
		panic(err)
	}

	// Resize image to 128x64 using Lanczos resampling
	resized := resize.Resize(uint(template.Images[1].Width), uint(template.Images[1].Height), img, resize.Lanczos3)

	// Encode resized image to PNG in-memory
	var buf bytes.Buffer
	err = png.Encode(&buf, resized)
	if err != nil {
		panic(err)
	}

	// Get []byte
	imageBytes := buf.Bytes()

	template.Images[0].Pixels = imageBytes*/


	// Load the image file
	filePath := "test1.png" // change to your image path
	img, err := loadImage(filePath)
	if err != nil {
		log.Fatal(err)
	}

	// Resize to 128x64
	resizedImg := imaging.Resize(img, int(template.Images[1].Width), int(template.Images[1].Height), imaging.Lanczos)

	// Convert to RGBA
	rgbaImg := toRGBA(resizedImg)

	// Convert to []byte (RGBA)
	byteArray := rgbaImg.Pix


	template.Images[1].Pixels = byteArray

	assembly.AssembleImage(0, template)








	rgba := &image.RGBA {
        Pix:    template.Images[0].Pixels,
        Stride: template.Images[0].Width * 4,
        Rect:   image.Rect(0, 0, template.Images[0].Width, template.Images[0].Height),
    }

    // Create the output file
    outFile, err := os.Create("chungus.png")
    if err != nil {
        return //err
    }
    defer outFile.Close()

    // Encode the image to PNG
   /* return*/ png.Encode(outFile, rgba)


}