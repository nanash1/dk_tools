import pydkdcmp

def decomp_file(file):
    file_cntr = 0
    decoded = 0
    
    with open(file, "rb") as ifile:
            comp = ifile.read()
           
    while len(comp) > 0:
        
        decomp, length = pydkdcmp.decomp(comp)
        if length > 0:
            with open(str(file_cntr).zfill(3)+".bin", "wb") as ofile:
                ofile.write(decomp)
            decoded += length
        else:
            break
        comp = comp[length:]
        file_cntr += 1