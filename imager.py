from PIL import Image
import numpy as np
import csv
import os

TFT_MAGENTA = (255,0,255)
TFT_YELLOW = (255, 255,   0)
TFT_RED = (255,   0,   0)
TFT_GREEN = (0, 255,   0)
TFT_DARKGREEN = (0, 128,   0)
TFT_CYAN = (0, 255, 255)
TFT_PURPLE = (128,   0, 128)
TFT_BLUE = (0,   0, 255)
TFT_NAVY = (0,   0, 128)
TFT_GREENYELLOW = (180, 255,   0)

# pixels = [
#    [(54, 54, 54), (232, 23, 93), (71, 71, 71), (168, 167, 167)],
#    [(204, 82, 122), (54, 54, 54), (168, 167, 167), (232, 23, 93)],
#    [(71, 71, 71), (168, 167, 167), (54, 54, 54), (204, 82, 122)],
#    [(168, 167, 167), (204, 82, 122), (232, 23, 93), (54, 54, 54)]
# ]

# # Convert the pixels into an array using numpy
# array = np.array(pixels, dtype=np.uint8)

# # Use PIL to create an image from the new array of pixels
# new_image = Image.fromarray(array)
# new_image.save('new.png')

print(os.listdir("./data/raw"))


def generate_image(filename):
    with open("./data/raw/{filename}".format(filename=filename), newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            values = row[0].split(",")
            #print(len(values))
            converted = list(map(lambda x: float(x), values))
            #print(converted)
            j=0
            r = []
            t = []
            for i in range(len(converted)):

                rgb = None
                val = converted[i]
                if val > 32.0:
                    rgb = TFT_MAGENTA
                elif val > 29.0:
                    rgb = TFT_RED
                elif val > 26.0:
                    rgb = TFT_YELLOW
                elif val > 20.0:
                    rgb = TFT_GREENYELLOW
                elif val > 17.0:
                    rgb = TFT_GREEN
                elif val > 10.0:
                    rgb = TFT_CYAN
                else:
                    rgb = TFT_BLUE
                
                for m in range(10):
                    r.append(rgb)
                
                j = j + 1
                if j == 32:
                    j=0
                    for m in range(10):
                        t.append(r)
                    
                    r = []

            #print(t)
            #print(len(t))
            array = np.array(t, dtype=np.uint8)
            new_image = Image.fromarray(array)
            new_image.save("./data/visual/{filename}.png".format(filename=filename))
        #print(', '.join(row))


if __name__ == "__main__":

    for filename in os.listdir("./data/raw"):
        generate_image(filename)
