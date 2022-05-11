import base64
def request_handler(request):
    """
    This function handles server requests. It takes a base 64 image string, decodes it
    and writes the image to a jpg file which can then be opened.
    """
    imagebase64 = request['data'][10:-2]
    decoded_image = base64.b64decode(imagebase64)
    filename = '/var/jail/home/aoejile/final_project/608image.jpg'
    with open(filename, 'wb') as f:
        f.write(decoded_image)
    print("successfully written file!")
        
