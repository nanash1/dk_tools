import pydkdcmp

def decomp_file(file, strip_header=True):
    '''
    Decompresses CHR files

    Parameters
    ----------
    file : string
        CHR file to decompress.
    strip_header : bool, optional
        Remove the 6 byte header. The default is True.

    Returns
    -------
    None.

    '''
    skip = 0
    if strip_header:
        skip = 6
    
    file_cntr = 0
    decoded = 0
    
    with open(file, "rb") as ifile:
            comp = ifile.read()
           
    while len(comp) > 0:
        
        decomp, length = pydkdcmp.decomp(comp)
        if length > 0:
            with open(str(file_cntr).zfill(4)+".bin", "wb") as ofile:
                ofile.write(decomp[skip:])
            decoded += length
        else:
            break
        comp = comp[length:]
        file_cntr += 1