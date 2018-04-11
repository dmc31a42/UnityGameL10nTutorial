using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[System.Serializable]
public class SerializeSubClass
{
    public byte byte1;
    public int int1;
    public string string1;
}

public class UnitySerializationExam : MonoBehaviour {

    public byte byte1;
    public byte byte2;
    public ushort ushort1;
    public uint uint1;
    public ulong ulong1;
    public sbyte sbyte1;
    public short short1;
    public int int1;
    public long long1;
    public float float1;
    public double double1;
    public decimal decimal1;
    public string string1;
    //
    public bool bool1;
    public UnitySerializationExam UnitySerializationExam1;
    public SerializeSubClass SerializeSubClass1;
    public byte[] bytes1;
    public int[] ints1;
    public SerializeSubClass[] SerializeSubClasses1;


    // Use this for initialization
    void Start () {
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
