Scriptname sot_genesispotions extends ReferenceAlias  


Int Property inout Auto
int whichone

Event OnOpen(ObjectReference akActionRef)
  if (akActionRef == Game.GetPlayer()) && inout == 0 && SOT_BeefierLoot.GetValueInt() == 0 && Utility.RandomInt(1, 100) <= GenesisLootChance.getvalueint()
  inout = 5

;Debug.Notification("ON OPEN pOTIONS!")

if Utility.RandomInt(1, 100) <= GenesisLootChance.getvalueint()
self.makepotion(self.GetReference())
EndIf
if Utility.RandomInt(1, 100) <= GenesisLootChance.getvalueint()
self.makescroll(self.GetReference())
EndIf
if Utility.RandomInt(1, 100) <= GenesisLootChance.getvalueint()
self.makemiscitem(self.GetReference())
EndIf
  endIf
endEvent

Potion Function MakePotion (ObjectReference Box)
Int JIndex = PotionList.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				potion potiontospawn = PotionList.GetAt(Jrandom) As Potion
				Box.AddItem(potiontospawn)
EndFunction

Scroll Function MakeScroll (ObjectReference Box)
Int JIndex = ScrollList.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				scroll scrolltospawn = ScrollList.GetAt(Jrandom) As Scroll
				Box.AddItem(scrolltospawn)
EndFunction

MiscObject Function MakeMiscItem (ObjectReference Box)
Int JIndex = MiscLoot.GetSize()
				Int JRandom = Utility.RandomInt(0, JIndex)
				MiscObject MiscObjecttospawn = MiscLoot.GetAt(Jrandom) As MiscObject
				Box.AddItem(MiscObjecttospawn)
EndFunction

FormList Property PotionList  Auto  

FormList Property ScrollList  Auto  

FormList Property MiscLoot  Auto  

GlobalVariable Property SOT_BeefierLoot  Auto  

GlobalVariable Property GenesisLootChance  Auto  

