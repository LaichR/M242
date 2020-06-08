import os, pathlib


def RewriteCsv( fname, outName ):
    with open( fname, "r") as input:
        lines = input.readlines()
        lineNr = 0
        
        values = lines[lineNr].split(',')
        while values[0] != 'Sample-Nr':
            lineNr+=1
            values = lines[lineNr].split(',')
            
        rewrittenLines = [lines[lineNr]]
        
        for l in lines[lineNr+1:]:
            values = l.split(',')
            lineStr = "{0},{1}\n".format(int(values[0],16), int(values[1],16))
            rewrittenLines.append(lineStr)
            
        with open( outName, "w") as output:
            output.writelines( rewrittenLines )
            
if __name__ == "__main__":
    RewriteCsv("Intensity_0.csv", "Intensity_0_dec.csv" )
    RewriteCsv("Intensity_4.csv", "Intensity_4_dec.csv" )
    RewriteCsv("Intensity_8.csv", "Intensity_8_dec.csv" )
    RewriteCsv("Intensity_12.csv", "Intensity_12_dec.csv" )
    RewriteCsv("Intensity_16.csv", "Intensity_16_dec.csv" )
    RewriteCsv("Intensity_72.csv", "Intensity_72_dec.csv" )
    RewriteCsv("Intensity_100.csv", "Intensity_100_dec.csv" )
    RewriteCsv("Intensity_108.csv", "Intensity_108_dec.csv" )