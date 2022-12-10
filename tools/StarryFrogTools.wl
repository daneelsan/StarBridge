(* ::Package:: *)

BeginPackage["StarryFrogTool`"];

StarryFrogConstellationDraw;

Begin["`Private`"];

$width = 11;
$height = 15;
$maxEdgeCount = 20;

toCString[data_] :=
	MapApply[
		Function[{p1, p2, bool},
			"        " <> StringRiffle[
				ToString /@ {p1[[1]], $height - 1 - p1[[2]], p2[[1]], $height - 1 - p2[[2]], If[bool, "BRIDGE_ON_DEFAULT", "BRIDGE_OFF_DEFAULT", "BRIDGE_DISABLED"]},
				{"{ ", ", ", " }"}
			]
		],
		PadRight[data, $maxEdgeCount, {{{-1, 15}, {-1, 15}, -1}}]
	] // StringRiffle[
		#,
		{"    {\n", ",\n", "\n    }"}
	] & // StringJoin[
		"{\n",
		"    " <> ToString[Length[data]] <> ",\n",
		"    " <> ToString[Length[Select[data, TrueQ[#[[3]]] &]]] <> ",\n",
		#,
		"\n}"
	] &;

StarryFrogConstellationDraw[dataIn_ : {}] :=
	DynamicModule[{data, p1, p2, mouseDraggedQ, undoValues, stars, nb},
		data = dataIn;
		p1 = p2 = {0, 0};
		mouseDraggedQ = False;
		undoValues = {};
		stars = Table[Tooltip[Point[{x, y}], {x, $height - 1 - y}], {x, 0, $width - 1, 1}, {y, 0, $height - 1, 1}];

		DialogInput[
			Column[{
				Row[{
					Button[
						"Print C structure",
						Print[toCString[data]]
					],
					Button[
						"Clear",
						data = {}
					],
					Button[
						"Reset",
						data = dataIn
					]
				}],
				Row[{
					Row[{"Number of vertices: ", Dynamic @ Length[Union[Catenate[data[[All, {1, 2}]]]]]}],
					Row[{"Number of edges: ", Dynamic @ Length[data], " (max. ", $maxEdgeCount, ")"}]
				}, " | "],
				EventHandler[
					Pane @ Graphics[{
						{Thick, Dynamic @ MapApply[{If[#3, Yellow, Red], Line[{#1, #2}]} &, data]},
						{Opacity[.5, White], PointSize[Large], stars}
					},
					Background -> Black
				],
				{
					"MouseDragged" :> (
						If[!mouseDraggedQ,
							p1 = Round @ MousePosition["Graphics"];
							mouseDraggedQ=True
							,
							p2 = Round @ MousePosition["Graphics"]
						]
					),
					"MouseUp" :> (
						mouseDraggedQ = False;
						AppendTo[data, {p1, p2, CurrentValue["ShiftKey"]}]
					)
				}]
			}],
			NotebookEventActions -> {
				"EscapeKeyDown" :> DialogReturn[data],
				{"MenuCommand", "Undo"} :>
					If[data =!= {},
						AppendTo[undoValues, Last[data]];
						data = Most[data];
						,
						Beep[]
					],
				{"MenuCommand", "Redo"} :>
					If[undoValues =!= {},
						AppendTo[data, Last[undoValues]];
						undoValues = Most[undoValues];
						,
						Beep[]
					]
			},
			WindowTitle -> "StarryFrogConstellationDraw",
			WindowElements -> {"StatusArea"}
		]
	]

End[]

EndPackage[];
