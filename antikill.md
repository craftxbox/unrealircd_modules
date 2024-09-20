# third/antikill
This module adds an immune:kill permission, which allows you to set operclasses as immune to being killed by remote servers.  
You can still be killed by ops on *your* server, but not by ops across the network.  
## ⚠️ This module does not work on stock unrealircd. It requires a hooktype that is only present in [PissIRCd](https://github.com/pissnet/pissircd)  
  
# Instructions
Place [antikill.c](/antikill.c) into src/modules/third of your PissIRCd source.  
`$ make && make install`  
Edit your .conf files to add `immune:kill` to applicable operclasses.  
NOTE: As this permission is in the already existing `immune` permission class, the default operclasses will have this by default!  
  
# Why this exists
About a year or two ago, relatively early in the history of pissnet, there were was some drama and malice going around on the network.  
An oper had created a pseudoservice or bot of some sort to /KILL anyone they wanted, any time they attempted to connect to the network.  
This was, incredibly disruptive, as an endless /KILL was almost impossible to combat, you couldn't just E-Line yourself from it like you could a G/Z-line.  
So, as a result, this module was created as a counter to this type of attack.  
  
# License

```
MIT License

Copyright (c) 2023 craftxbox

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.```
