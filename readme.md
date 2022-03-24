# Vectrex DIY Cart+

An extension to the Vectrex multicart 'DIY Cart+' by Alan Leverett that adds:

- Scrolling ROM names
- Improved Arduino code

The mutlicart promotes tinkering and exploring hardware-based bankswitching of ROMs via an Arduino Nano.

You are encouraged to change the code, and create your own EPROM images.

This GitHub repo does not itself include any ROM images for the EPROM.

For instructions on printing, assembling and programming, see links below.

## Configure Code

The arduino code is currently targetting:

- 27c040 / 27c4001 EPROM for the +8 Cart (35 games in Alan's EPROM image)
- 27c080 / 27c801 EPROM for +32 Cart (21 games in Alan's EPROM image)


Set 'IS_8K_CART' to 'true' for the +8 Cart, and set it to 'false' for the +32 Cart.

Update 'ROM_MAX' and the 'title[]' String array if you change the games on the EPROM. 

## Links

- [Alan Leverett DIY Cart+ Home](https://www.levosretrocomputerprojects.co.uk/index.php/vectrex/13-diy-cart)
- [Print DIY Cart+32 PCB](https://www.pcbway.com/project/shareproject/DIYCart_32.html)
- [Print DIY Cart+8 PCB](https://www.pcbway.com/project/shareproject/DIY_Cart_8.html)
