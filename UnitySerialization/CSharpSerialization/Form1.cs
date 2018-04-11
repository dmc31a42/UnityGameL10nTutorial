using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

namespace CSharpSerialization
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private LoanClass.Loan TestLoan = new LoanClass.Loan(10000.0, 0.075, 36, "Neil Black", new int[]{1, 2, 3, 4});
        const string FileName = @"..\..\SavedLoan.bin";

        private void Form1_Load(object sender, EventArgs e)
        {
            if (File.Exists(FileName))
            {
                Stream TestFileStream = File.OpenRead(FileName);
                BinaryFormatter deserializer = new BinaryFormatter();
                TestLoan = (LoanClass.Loan)deserializer.Deserialize(TestFileStream);
                TestFileStream.Close();
            }

            TestLoan.PropertyChanged += this.CustomerPropertyChanged;

            textBox1.Text = TestLoan.LoanAmount.ToString();
            textBox5.Text = ConvertBytes2String(BitConverter.GetBytes(TestLoan.LoanAmount));
            textBox2.Text = TestLoan.InterestRate.ToString();
            textBox6.Text = ConvertBytes2String(BitConverter.GetBytes(TestLoan.InterestRate));
            textBox3.Text = TestLoan.Term.ToString();
            textBox7.Text = ConvertBytes2String(BitConverter.GetBytes(TestLoan.Term));
            textBox4.Text = TestLoan.Customer;
            textBox8.Text = ConvertBytes2String(BitConverter.GetBytes(TestLoan.Customer.Length));
            textBox9.Text = "";
            foreach(int item in TestLoan.IntArray)
            {
                textBox9.Text += item.ToString() + " ";
            }
        }

        private void CustomerPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            MessageBox.Show(e.PropertyName + " has been changed.");
        }

        private string ConvertBytes2String(byte[] bytes)
        {
            string str = "";
            foreach(byte item in bytes)
            {
                str += item.ToString("X2") + " ";
            }
            return str;
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            TestLoan.LoanAmount = Convert.ToDouble(textBox1.Text);
            TestLoan.InterestRate = Convert.ToDouble(textBox2.Text);
            TestLoan.Term = Convert.ToInt32(textBox3.Text);
            TestLoan.Customer = textBox4.Text;
            TestLoan.IntArray = textBox9.Text.Trim().Split(' ').Select(n => Convert.ToInt32(n)).ToArray();


            Stream TestFileStream = File.Create(FileName);
            BinaryFormatter serializer = new BinaryFormatter();
            serializer.Serialize(TestFileStream, TestLoan);
            TestFileStream.Close();
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            textBox5.Text = ConvertBytes2String(BitConverter.GetBytes(Convert.ToDouble(textBox1.Text)));
        }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
            textBox6.Text = ConvertBytes2String(BitConverter.GetBytes(Convert.ToDouble(textBox2.Text)));
        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {
            textBox7.Text = ConvertBytes2String(BitConverter.GetBytes(Convert.ToInt32(textBox3.Text)));
        }

        private void textBox4_TextChanged(object sender, EventArgs e)
        {
            textBox8.Text = ConvertBytes2String(BitConverter.GetBytes(textBox4.Text.Length));
        }

        private void Save_Click(object sender, EventArgs e)
        {
            Form1_FormClosing(null, null);
            Form1_Load(null, null);
        }

        private void textBox9_TextChanged(object sender, EventArgs e)
        {
            if (textBox9.Text != String.Empty)
            {
                int[] tempIntArray = textBox9.Text.Trim().Split(' ').Select(n => Convert.ToInt32(n)).ToArray();
                Byte[] tempByteArray = new Byte[tempIntArray.Length * 4];
                Buffer.BlockCopy(tempIntArray, 0, tempByteArray, 0, tempIntArray.Length * 4);
                textBox10.Text = "";
                foreach (Byte item in tempByteArray)
                {
                    textBox10.Text += item.ToString("X2") + " ";
                }
            }
        }
    }
}
