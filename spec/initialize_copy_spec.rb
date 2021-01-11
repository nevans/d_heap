# frozen_string_literal: true

RSpec.describe DHeap do
  # ruby 3.0+
  init_clone_kwarg = Object.instance_method(:initialize_clone).arity == -1

  shared_examples "private method defined on DHeap" do |method|
    it "is a private instance method" do
      expect(DHeap.private_instance_methods).to include(method)
    end

    it "is defined on DHeap" do
      expect(DHeap.instance_method(method).owner).to eq(DHeap)
    end
  end

  describe "#initialize_copy" do
    include_examples "private method defined on DHeap", :initialize_copy
  end

  if init_clone_kwarg
    describe "#initialize_clone" do
      include_examples "private method defined on DHeap", :initialize_clone
    end
  end

  shared_examples "unfrozen copy" do |copier, freeze_orig: true|
    it "creates an unfrozen copy" do
      orig = DHeap.new
      orig << 1 << 2 << 3
      orig.freeze if freeze_orig
      copy = copier.call(orig)

      expect(copy).to_not be_frozen
      expect(copy.pop).to eq(1)

      expect(copy.size).to be(2)
      expect(orig.size).to be(3)
    end
  end

  describe "#dup" do
    include_examples "unfrozen copy", -> orig { orig.dup }, freeze_orig: true
  end

  shared_examples "frozen copy" do |copier, freeze_orig: true|
    it "creates a frozen copy" do
      orig = DHeap.new
      orig << 1 << 2 << 3
      orig.freeze if freeze_orig
      copy = copier.call(orig)

      expect(copy).to be_frozen
      expect { copy.pop }.to raise_error(FrozenError)

      expect(copy.size).to be(3)
      expect(orig.size).to be(3)
    end
  end

  describe "#clone" do
    context "with an unfrozen original heap" do
      include_examples "unfrozen copy", -> orig { orig.clone }, freeze_orig: false
    end

    context "with a frozen original heap" do
      include_examples "frozen copy", -> orig { orig.clone }, freeze_orig: true
    end

    # different meanings:
    #   in 2.x: true copies frozen state (like nil in 3.0)
    #   in 3.x: true freezes copy
    context "freeze: true" do
      if init_clone_kwarg
        context "with an unfrozen original heap" do
          include_examples("frozen copy",
                           -> orig { orig.clone freeze: true },
                           freeze_orig: false)
        end
      else
        context "with an unfrozen original heap" do
          include_examples("unfrozen copy",
                           -> orig { orig.clone freeze: true },
                           freeze_orig: false)
        end
      end

      context "with a frozen original heap" do
        include_examples("frozen copy",
                         -> orig { orig.clone freeze: true },
                         freeze_orig: true)
      end
    end

    context "freeze: false" do
      context "with an unfrozen original heap" do
        include_examples("unfrozen copy",
                         -> orig { orig.clone freeze: false },
                         freeze_orig: false)
      end

      context "with a frozen original heap" do
        include_examples("unfrozen copy",
                         -> orig { orig.clone freeze: false },
                         freeze_orig: true)
      end
    end
  end

end
