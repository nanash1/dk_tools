import pydkdcmp, os, fnmatch

def _find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result

def decomp_file(file, strip_header=True):
    '''
    Decompresses a CHR file

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
        
    with open(file, "rb") as ifile:
        comp = ifile.read()
        
    decomp, length = pydkdcmp.decomp(comp)
    if length > 0:
        fname, ext = os.path.splitext(file)
        with open(fname+".bin", "wb") as ofile:
            ofile.write(decomp[skip:])
    else:
        raise ValueError("Invalid file size")
        
def comp_file(file):
    '''
    Compresses files to CHR

    Parameters
    ----------
    file : string
        File to decompress.

    Returns
    -------
    None.

    '''
    
    with open(file, "rb") as ifile:
        data = ifile.read()
            
    comp = pydkdcmp.comp(data)
    
    if len(comp) < len(data):
        comp = b'\x01'+len(data).to_bytes(2, 'little', signed=False)+comp
    else:
        comp = b'\x00'+len(data).to_bytes(2, 'little', signed=False)+data
           
    fname, ext = os.path.splitext(file)
    
    with open(fname+".CHR", "wb") as ofile:
        ofile.write(comp)
        
def split(file, decompress=True, strip_header=False):
    """
    Splits a CHR file into single files with optional decompression.

    Parameters
    ----------
    file : str
        File to split.
    decompress : bool, optional
        Apply decompression. The default is True.
    strip_header : bool, optional
        Strip image headers. The default is False.

    Returns
    -------
    None.

    """
    file_cntr = 0
    
    with open(file, "rb") as ifile:
            comp = ifile.read()
            
    offset_tbl = b''
    offset = 0
           
    while len(comp) > 0:
        
        decomp, length = pydkdcmp.decomp(comp)
        if length > 0:
            
            with open(str(file_cntr).zfill(4)+".CHR", "wb") as ofile:
                ofile.write(comp[:length])
            
            if decompress:
                decomp_file(str(file_cntr).zfill(4)+".CHR", strip_header)
                
            offset_tbl += offset.to_bytes(4, 'big', signed=False)
            offset += length
            
        else:
            break
        comp = comp[length:]
        file_cntr += 1
        
    fname, ext = os.path.splitext(file)
    with open(fname+".tbl", "wb") as ofile:
        ofile.write(offset_tbl)
        
def combine(dir):
    """
    Combines files in a folder and generates the corresponding offset table.

    Parameters
    ----------
    dir : str
        Path to folder.

    Returns
    -------
    None.

    """
    if not os.path.isdir(dir):
        raise ValueError("Input not a directory")
        
    offset_tbl = b''
    offset = 0
        
    with open(dir+os.path.dirname(dir)+".TMP", "wb") as ofile:
        
        for f in _find('*.CHR', dir):
            with open(f, 'rb') as ifile:
                fdata = ifile.read()
                
                offset_tbl += offset.to_bytes(4, 'big', signed=False)
                offset += len(fdata)
                
                ofile.write(fdata)
                
    os.rename(dir+os.path.dirname(dir)+".TMP", dir+os.path.dirname(dir)+".CHR")
                
    with open(dir+"offset.tbl", "wb") as ofile:
        ofile.write(offset_tbl)
        
    
    