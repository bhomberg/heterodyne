using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ButtonGridController : MonoBehaviour {

	public AudioSource castleVoice;
	public AudioClip introClip;
	public AudioClip winClip;
	public AudioClip overheatingClip;
	public AudioClip tooManyRedClip;
	public AudioClip tooManyOrangeClip;
	public AudioClip tooManyYellowClip;
	public AudioClip tooManyGreenClip;
	public AudioClip tooManyBlueClip;
	public AudioClip tooManyPurpleClip;
	public AudioClip tooManyBlackClip;
	public AudioClip tooManyWhiteClip;

	public Button clueButton;
	public Text castleText;
	public Text clueText;

	public int gridSize;
	public string board;
	private List<bool> state;

	public GameObject buttonGrid;

	public Texture2D tex;
	private List<GameObject> gameObjects;

	private List<int> illuminationCounts;
	private List<int> walls;
	private Dictionary<int, Pair<int, string>> wallConstraints;  // maps index to (count, colorName)
	private List<string> clues;
	int nextClue;

	private Pair<int, int>[] directions = {
		new Pair<int, int> (0, 1),
		new Pair<int, int> (1, 0),
		new Pair<int, int> (0, -1),
		new Pair<int, int> (-1, 0),
	};
	private Pair<Color, string>[] colors = {
		new Pair<Color, string>(Color.red, "red"),
		new Pair<Color, string>(new Color(1, 0.647f ,0), "orange"),
		new Pair<Color, string>(new Color(1, 1, 0), "yellow"),
		new Pair<Color, string>(Color.green, "green"),
		new Pair<Color, string>(new Color(94.0f/255, 151.0f/255, 242.0f/255), "blue"),
		new Pair<Color, string>(new Color(0.594f, 0, 0.51f), "purple"),
		new Pair<Color, string>(Color.white, "white"),
		new Pair<Color, string>(Color.black, "black"),
	};

	private string welcomeMessage = "Castle Heterodyne: **Grumble grumble** I've trapped you in the dark, stranger! You can try turning the lights back on, but I'm very picky about my lighting.";
	private string tooManyMessage = "Castle Heterodyne: **Grumble grumble** I don't like the lights. You've turned on too many {0} switches.";
	private string tooFewMessage = "Castle Heterodyne: **Grumble grumble**  I don't like the lights. You've turned on too few {0} switches.";
	private string winMessage = "Castle Heterodyne: **gasp** That lighting is... beautiful. Maybe you're not so bad after all. (You win)";
	private string overheatingMessage = "Castle Heterodyne: Ahhhhhh! The grid's overheating. Turn off those glowy red lights NOW or you'll catch this place on fire!";

	// Use this for initialization
	void Start () {
		

		wallConstraints = new Dictionary<int, Pair<int, string>> ();
		clueButton.onClick.AddListener(ShowClue);
	
		showMessage(welcomeMessage);
		clueText.text = "";

		clues = new List<string> ();
		walls = new List<int> ();
		gameObjects = new List<GameObject> ();
		state = new List<bool> ();
		illuminationCounts = new List<int> ();

		ParseBoardString (board);

		for (int i = 0; i < gridSize * gridSize; i++) {
			int row = GetRow (i);
			int col = GetCol (i);

			GameObject newObj = new GameObject ();
			newObj.name = string.Format ("Game Object #{0}: ({1}, {2})", i, row, col);
			newObj.AddComponent<SpriteRenderer>();
			SpriteRenderer renderer = newObj.GetComponent<SpriteRenderer>();
			Sprite s = Sprite.Create (tex, new Rect (0.0f, 0.0f, tex.width, tex.height), new Vector2 (0.5f, 0.5f));
			renderer.sprite = s;

			newObj.AddComponent<BoxCollider2D> ();
			newObj.transform.parent = buttonGrid.transform;
			newObj.transform.position = GetVector2 (row, col);  // row 0, col 0 is lower left.
			newObj.tag = "Button";

			gameObjects.Add (newObj);
			state.Add (false);
			illuminationCounts.Add (0);
		}


		Shuffle (clues);
		clues.Insert (0, "Light up ALL the lights.");
		nextClue = 0;

		Render ();
	}

	// Update is called once per frame
	void Update () {
		if (Input.GetMouseButtonDown (0)) {
			Vector2 mousePosition = Camera.main.ScreenToWorldPoint (Input.mousePosition);
			Collider2D hitCollider = Physics2D.OverlapPoint (mousePosition);

			if (hitCollider && hitCollider.tag == "Button") {
				int y = (int)hitCollider.transform.position.y;
				int x = (int)hitCollider.transform.position.x;
				int row = y + gridSize / 2;
				int col = x + gridSize / 2;
				int index = GetIndex (row, col);
				Debug.Log (string.Format ("Clicked #{0} ({1}, {2})", index, row, col));

				if (IsWall (row, col)) {
					// Do nothing...
					return;
				}
				else if (state [index]) {
					TurnOff (index);
				} else {
					TurnOn (index);
				}

				Render ();
				CheckResult ();
			}
		}
	}

	private void CheckResult() {
		bool ok = true;
		bool full = true;
		string fullBoardMessage = "";
		for (int index = 0; index < gridSize * gridSize; index++) {
			if (walls.Contains (index)) {
				Pair<int, string> constraint = wallConstraints [index];
				int targetCount = constraint.First;
				string colorName = constraint.Second;
				int count = 0;
				foreach (Pair<int, int> direction in directions) {
					int row = GetRow (index) + direction.First;
					int col = GetCol (index) + direction.Second;
					if (!InBounds (row, col)) {
						continue;
					}
					int neighborIndex = GetIndex (row, col);
					if (state [neighborIndex]) {
						count += 1;
					}
				}

				if (count > targetCount) {
					ok = false;
					fullBoardMessage = string.Format (tooManyMessage, colorName);
					// Debug.Log (fullBoardMessage);
				} else if (count < targetCount) {
					ok = false;
					fullBoardMessage = string.Format (tooFewMessage, colorName);
					// Debug.Log (fullBoardMessage);
				}
			} else if (illuminationCounts [index] < 1) {
				ok = false;
				full = false;
			} else if (illuminationCounts [index] > 1 && state [index]) {
				ok = false;
				showMessage (overheatingMessage);
				return;
			}
		}
		if (castleText.text == overheatingMessage) {
			// If the user fixed the overheating problem, shut up.
			castleVoice.Stop ();
		}
		if (ok) {
			Debug.Log (string.Format("Win at: {0}", Time.time));
			showMessage (winMessage);
		} else if (full) {
			Debug.Log (string.Format("Full at: {0}", Time.time));
			showMessage (fullBoardMessage);
		} else if (castleText.text != welcomeMessage) {
			showMessage ("");
		}
	}

	private void showMessage(string message) {
		castleText.text = message;

		if (message == welcomeMessage) {
			PlayOneShot (introClip);
		} else if (message == overheatingMessage) {
			if (!castleVoice.isPlaying) {
				PlayOneShot (overheatingClip);
			}
		} else if (message == winMessage) {
			PlayOneShot (winClip);
		} else if (message == string.Format (tooManyMessage, "red")) {
			PlayOneShot (tooManyRedClip);
		} else if (message == string.Format (tooManyMessage, "orange")) {
			PlayOneShot (tooManyOrangeClip);
		} else if (message == string.Format (tooManyMessage, "yellow")) {
			PlayOneShot (tooManyYellowClip);
		} else if (message == string.Format (tooManyMessage, "blue")) {
			PlayOneShot (tooManyBlueClip);
		} else if (message == string.Format (tooManyMessage, "green")) {
			PlayOneShot (tooManyGreenClip);
		} else if (message == string.Format (tooManyMessage, "purple")) {
			PlayOneShot (tooManyPurpleClip);
		} else if (message == string.Format (tooManyMessage, "black")) {
			PlayOneShot (tooManyBlackClip);
		} else if (message == string.Format (tooManyMessage, "white")) {
			PlayOneShot (tooManyWhiteClip);
		}

		Debug.Log (castleVoice.clip.length);

	}

	private void PlayOneShot(AudioClip clip) {
		castleVoice.clip = clip;
		castleVoice.Play ();
	}

	private void ShowClue() {
		clueText.text = clues [nextClue];
		nextClue++;
		nextClue %= clues.Count;
	}

	private void Render() {
		for (int i = 0; i < gridSize * gridSize; i++) {
			GameObject g = gameObjects [i];
			SpriteRenderer r = g.GetComponent<SpriteRenderer> ();
			if (IsWall (GetRow (i), GetCol (i))) {
				r.color = Color.black;
				r.transform.localScale = new Vector3 (1.3f, 1.3f, 0);
			} else if (state [i]) {
				if (illuminationCounts [i] > 1) {
					r.color = Color.red;
				} else {
					r.color = new Color (1, 1, 0); // new Color (1, 0.647f, 0); //orange. //new Color(1, 1, 0.2f); // light you've turned on
				}
			} else {
				if (illuminationCounts [i] > 0) {
					r.color = 2 * Color.yellow / 3;
				} else {
					r.color = Color.white;
				}
			}
		}
	}

	private void ParseBoardString(string board) {
		string[] lines = board.Split (' ');
		int row = gridSize;
		foreach (string line in lines) {
			row -= 1;
			int col = 0;
			foreach (char c in line) {
				int index = GetIndex (row, col);
				if ("01234*".Contains (c.ToString())) {
					walls.Add (index);
				}
				if ("01234".Contains (c.ToString())) {
					int count = int.Parse (c.ToString ());
					AddWallConstraint (index, count);
				}
				col += 1;
			}
		}
	}

	private void AddWallConstraint(int index, int count) {
		Pair<Color, string> namedColor = colors [(walls.Count - 1) % colors.Length];
		Color color = namedColor.First;
		Debug.Log (string.Format ("Walls {0} Color: {1} CLen {2} Index {3}  Row, Col: {4} {5}", walls.Count, color, colors.Length, index, GetRow (index), GetCol (index)));

		wallConstraints.Add(index, new Pair<int, string>(count, namedColor.Second));
		foreach (Pair<int, int> direction in directions) {
			int row = GetRow (index) + direction.First;
			int col = GetCol (index) + direction.Second;
			if (!InBounds (row, col)) {
				continue;
			}
			GameObject newObj = new GameObject ();
			newObj.name = string.Format ("Background of #{0} ({1}, {2})", GetIndex (row, col), row, col);
			newObj.transform.parent = buttonGrid.transform;
			newObj.transform.position = GetVector2 (row, col);
			newObj.transform.localScale = new Vector3 (2, 2);
			newObj.AddComponent<SpriteRenderer> ();
			SpriteRenderer renderer = newObj.GetComponent<SpriteRenderer> ();
			Sprite s = Sprite.Create (tex, new Rect (0.0f, 0.0f, tex.width, tex.height), new Vector2 (0.5f, 0.5f));
			renderer.sprite = s;
//			if (namedColor.Second != "white") {
//				color.a = 0.3f;
//			} else {
//				color.a = 0.5f;
//			}
			renderer.color = color;
		}

		clues.Add (string.Format ("You found a clue: 'Flip {0} {1} switch(es).'", count, namedColor.Second));
	}

	private void TurnOn(int index) {
		state [index] = true;
		foreach (int light in Illuminates(index)) {
			illuminationCounts [light] += 1;
			// if (illuminationCounts[light] > 1 && state[light]) {we have a problem!}
		}
	}

	private void TurnOff(int index) {
		state [index] = false;
		foreach (int light in Illuminates(index)) {
			illuminationCounts [light] -= 1;
		}
	}

	private List<int> Illuminates(int index) {
		List<int> result = new List<int> ();
		int row_initial = GetRow (index);
		int col_initial = GetCol (index);

		result.Add (index);
		foreach (Pair<int, int> direction in directions) {
			int row = row_initial + direction.First;
			int col = col_initial + direction.Second;
			while (!IsWall (row, col)) {
				result.Add (GetIndex (row, col));
				row += direction.First;
				col += direction.Second;
			}
		}
		return result;
	}

	private bool InBounds(int row, int col) {
		return row >= 0 && col >= 0 && row < gridSize && col < gridSize;
	}

	private bool IsWall(int row, int col) {
		if (!InBounds(row, col)) {
			return true;
		}
		return walls.Contains (GetIndex (row, col));
	}

	private int GetIndex(int row, int col) {
		return row * gridSize + col;
	}

	private int GetRow(int index) {
		return index / gridSize;
	}

	private int GetCol(int index) {
		return index % gridSize;
	}

	private Vector2 GetVector2(int row, int col) {
		return new Vector2 (col - gridSize / 2, row - gridSize / 2);
	}

	private void Shuffle<T>(List<T> list)  
	{  
		int n = list.Count;  
		while (n > 1) {  
			n--;  

			int k = (int)(Random.value * (n + 1));  
			T value = list[k];  
			list[k] = list[n];  
			list[n] = value;  
		}  
	}
}


class Pair<T, U> {
	public Pair() {
	}

	public Pair(T first, U second) {
		this.First = first;
		this.Second = second;
	}

	public T First { get; set; }
	public U Second { get; set; }
};