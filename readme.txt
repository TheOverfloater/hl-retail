Half-Life SDK v2.3 for Microsoft Visual Studio 2008
Readme file
--------------------------------------------


OVERVIEW

Last night I spent a few hours to make Half-Life source code to be compiled in Visual Studio 2008. It was very boring and tiresome work, so I think, publishing the results of this work would be very useful for those coders who want to deal with Half-Life SDK and use MSVC 2008 (which is modern and advanced and which you can legally download for free, unlike MSVC 6).


ISSUES

- Both client and server projects included in 'src_dll' solution. The solution smoothly and clearly compiling on VC 2008 and probably output DLLs are workable. :)

- I removed original VC6 DSPs and I am not sure you will be able to compile the SDK in VC6 at all. But you still can restore them from original SDK if you want.

- 'ARRAYSIZE' macro in 'engine\eiface.h' undefines a macro with the same name which defined in 'winnt.h'

- A strange 'warning C4482' disabled in the client project (seems to be not serious)

- Original 'common\com_model.h' replaced with its analog from QuakeWorld to allow access map surfaces, polygons, textures etc.

- !!!IMPORTANT: 'HSPRITE' type on the client renamed to 'SpriteHandle_t' due to 'windef.h' which defined same-named type for itself. I hope you will like this new typename. :)


Good luck!
Gary_McTaggart,
Tesha Software
23.09.2009
