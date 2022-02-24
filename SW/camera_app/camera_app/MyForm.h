#pragma once

namespace cameraapp {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::ComboBox^ deviceSelect;
	public: System::Windows::Forms::Label^ outLabel;

	public:

	public:

	public:

	public:
	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->deviceSelect = (gcnew System::Windows::Forms::ComboBox());
			this->outLabel = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// deviceSelect
			// 
			this->deviceSelect->FormattingEnabled = true;
			this->deviceSelect->Location = System::Drawing::Point(12, 40);
			this->deviceSelect->Name = L"deviceSelect";
			this->deviceSelect->Size = System::Drawing::Size(365, 21);
			this->deviceSelect->TabIndex = 0;
			this->deviceSelect->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::deviceSelect_SelectedIndexChanged);
			// 
			// outLabel
			// 
			this->outLabel->AutoSize = true;
			this->outLabel->Location = System::Drawing::Point(196, 227);
			this->outLabel->Name = L"outLabel";
			this->outLabel->Size = System::Drawing::Size(48, 13);
			this->outLabel->TabIndex = 1;
			this->outLabel->Text = L"outLabel";
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(684, 485);
			this->Controls->Add(this->outLabel);
			this->Controls->Add(this->deviceSelect);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private: System::Void deviceSelect_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e) {

	}
	private: System::Void MyForm_Load(System::Object^ sender, System::EventArgs^ e) {
	}
};
}
